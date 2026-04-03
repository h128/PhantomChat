#include "../headers/DocumentProcessor.h"
#include "../headers/CorsHelper.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <phantomchat/events/Events.h>
#include <phantomchat/utils/FileProvider.hpp>
#include <phantomchat/utils/JsonSerialization.hpp>

namespace phantomchat::processors {

template<bool SSL>
std::jthread uploadDocumentBackgroundProcess(uWS::TemplatedApp<SSL> *app,
  moodycamel::BlockingConcurrentQueue<UploadTask<SSL>> &task_queue)
{
  auto worker_loop = [&task_queue, app] {
    while (uploadProcessorRunning.load()) {
      UploadTask<SSL> task;
      if (!task_queue.wait_dequeue_timed(task, std::chrono::milliseconds(1000))) continue;

      auto context_ptr = task.file_context.lock();
      if (!context_ptr || context_ptr->aborted) {
        // Context was destroyed or upload was aborted by client disconnect
        continue;
      }

      // Open file stream on first chunk
      if (!context_ptr->file_stream.is_open()) {
        context_ptr->file_stream.open(context_ptr->filename, std::ios::binary | std::ios::out);
        if (!context_ptr->file_stream) {
          // Failed to open file, close connection
          if (!context_ptr->aborted) { task.res->close(); }
          continue;
        }
      }

      // Write chunk to file
      context_ptr->file_stream.write(task.file_data.data(), static_cast<std::streamsize>(task.file_data.size()));

      if (!task.is_last_chunk) continue;
      context_ptr->file_stream.close();

      if (context_ptr->aborted) continue;

      app->getLoop()->defer([res = task.res, cors_origin = context_ptr->cors_origin] {
        res->writeStatus("200 OK");
        phantomchat::cors::writeHeaders(res, cors_origin);
        res->end("File uploaded successfully");
      });

      std::string just_filename = std::filesystem::path(context_ptr->filename).filename().string();

      phantomchat::events::FileUploadedEvent event(
        just_filename, context_ptr->room_name, context_ptr->user_uuid, context_ptr->is_poster);

      app->publish(event.room_name, json(event).dump(), uWS::OpCode::TEXT);
    }
  };

  return std::jthread(worker_loop);
}

template<bool SSL>
std::jthread downloadDocumentBackgroundProcess(uWS::TemplatedApp<SSL> *app,
  moodycamel::BlockingConcurrentQueue<DownloadTask<SSL>> &task_queue)
{
  static constexpr std::size_t chunk_size_bytes = 64U * 1024U;

  auto worker_loop = [&task_queue, app] {
    while (uploadProcessorRunning.load()) {
      DownloadTask<SSL> task;
      if (!task_queue.wait_dequeue_timed(task, std::chrono::milliseconds(1000))) continue;

      auto context_ptr = task.file_context.lock();
      if (!context_ptr || context_ptr->aborted) { continue; }

      // 1. Read file into buffer on background thread
      std::ifstream ifs(context_ptr->filename, std::ios::binary | std::ios::ate);
      if (!ifs) {
        app->getLoop()->defer([res = task.res, ctx = task.file_context]() {
          auto ptr = ctx.lock();
          if (ptr && !ptr->aborted) {
            res->writeStatus("404 Not Found");
            phantomchat::cors::writeHeaders(res, ptr->cors_origin);
            res->end("File not found");
          }
        });
        continue;
      }

      auto file_size = static_cast<std::size_t>(ifs.tellg());
      ifs.seekg(0, std::ios::beg);
      auto bytes = std::make_shared<std::vector<char>>(file_size);
      ifs.read(bytes->data(), static_cast<std::streamsize>(file_size));
      ifs.close();

      if (context_ptr->aborted) continue;

      std::string just_filename = std::filesystem::path(context_ptr->filename).filename().string();
      phantomchat::utils::FileProvider file_provider;
      std::string content_type(file_provider.mimeType(context_ptr->filename));

      app->getLoop()->defer(
        [res = task.res, bytes, just_filename, content_type, cors_origin = context_ptr->cors_origin]() {
          res->writeStatus("200 OK");
          phantomchat::cors::writeHeaders(res, cors_origin);
          res->writeHeader("Content-Type", content_type);
          res->writeHeader("Content-Disposition", "attachment; filename=\"" + just_filename + "\"");
          res->writeHeader("Transfer-Encoding", "chunked");

          auto offset = std::make_shared<std::size_t>(0U);
          auto finished = std::make_shared<bool>(false);

          res->onAborted([bytes, finished]() {
            *finished = true;
            bytes->clear();
            bytes->shrink_to_fit();
          });

          res->onWritable([res, bytes, offset, finished](std::uintmax_t) mutable {
            if (*finished) { return false; }
            while (*offset < bytes->size()) {
              const auto remaining = bytes->size() - *offset;
              const auto chunk_size = std::min<std::size_t>(chunk_size_bytes, remaining);
              const auto ok = res->write(std::string_view(bytes->data() + *offset, chunk_size));
              *offset += chunk_size;
              if (!ok) { return true; }
            }
            *finished = true;
            res->end();
            return false;// Stop writable events once done
          });

          // Initial write attempt
          while (*offset < bytes->size()) {
            const auto remaining = bytes->size() - *offset;
            const auto chunk_size = std::min<std::size_t>(chunk_size_bytes, remaining);
            const auto ok = res->write(std::string_view(bytes->data() + *offset, chunk_size));
            *offset += chunk_size;
            // If write returns false, wait for onWritable to continue sending(backpressure mode is active)
            if (!ok) { break; }
          }
          if (*offset >= bytes->size() && !*finished) {
            *finished = true;
            res->end();
          }
        });
    }
  };

  return std::jthread(worker_loop);
}

}// namespace phantomchat::processors

template std::jthread phantomchat::processors::uploadDocumentBackgroundProcess<false>(uWS::App *,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::UploadTask<false>> &);

template std::jthread phantomchat::processors::downloadDocumentBackgroundProcess<false>(uWS::App *,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::DownloadTask<false>> &);

template std::jthread phantomchat::processors::uploadDocumentBackgroundProcess<true>(uWS::SSLApp *,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::UploadTask<true>> &);

template std::jthread phantomchat::processors::downloadDocumentBackgroundProcess<true>(uWS::SSLApp *,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::DownloadTask<true>> &);
