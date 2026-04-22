#include "../headers/DocumentProcessor.h"
#include "../headers/ChunkedResponse.h"
#include "../headers/SecurityHeaders.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <phantomchat/events/Events.h>
#include <phantomchat/utils/FileProvider.hpp>
#include <phantomchat/utils/JsonSerialization.hpp>

namespace phantomchat::processors {

template<typename APP_TYPE, bool SSL>
std::jthread uploadDocumentBackgroundProcess(moodycamel::BlockingConcurrentQueue<UploadTask<APP_TYPE, SSL>> &task_queue,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &event_logger)
{
  auto worker_loop = [&task_queue, &event_logger](std::stop_token stop) {
    while (!stop.stop_requested()) {
      UploadTask<APP_TYPE, SSL> task;
      if (!task_queue.wait_dequeue_timed(task, std::chrono::milliseconds(1000))) continue;

      auto context_ptr = task.file_context.lock();
      if (!context_ptr || context_ptr->aborted) {
        // Context was destroyed or upload was aborted by client disconnect
        continue;
      }

      auto &app = context_ptr->app;

      // Open file stream on first chunk
      if (!context_ptr->file_stream.is_open()) {
        context_ptr->file_stream.open(context_ptr->filename, std::ios::binary | std::ios::out);
        if (!context_ptr->file_stream) {
          // Failed to open file, close connection
          app.getLoop()->defer([res = task.res] { res->close(); });
          continue;
        }
      }

      // Write chunk to file
      context_ptr->file_stream.write(task.file_data.data(), static_cast<std::streamsize>(task.file_data.size()));

      if (!task.is_last_chunk) continue;
      context_ptr->file_stream.close();

      if (context_ptr->aborted) continue;

      app.getLoop()->defer([&app, res = task.res, &event_logger, context_ptr] {
        res->writeStatus("200 OK");
        phantomchat::security::writeHeaders(res, context_ptr->cors_origin);
        res->end("File uploaded successfully");


        phantomchat::events::FileUploadedEvent event(std::filesystem::path(context_ptr->filename).filename().string(),
          context_ptr->room_name,
          context_ptr->user_uuid,
          context_ptr->is_poster);
        const auto json_event = json(event).dump();
        app.publish(event.room_name, json_event, uWS::OpCode::TEXT);

        event_logger.enqueue({ .room_name = context_ptr->room_name, .json_event = json_event });
      });
    }
  };

  return std::jthread(std::move(worker_loop));
}

template<typename APP_TYPE, bool SSL>
std::jthread downloadDocumentBackgroundProcess(
  moodycamel::BlockingConcurrentQueue<DownloadTask<APP_TYPE, SSL>> &task_queue)
{
  auto worker_loop = [&task_queue](std::stop_token stop) {
    while (!stop.stop_requested()) {
      DownloadTask<APP_TYPE, SSL> task;
      if (!task_queue.wait_dequeue_timed(task, std::chrono::milliseconds(1000))) continue;

      auto context_ptr = task.file_context.lock();
      if (!context_ptr || context_ptr->aborted) { continue; }

      auto &app = context_ptr->app;

      // 1. Read file into buffer on background thread
      std::ifstream ifs(context_ptr->filename, std::ios::binary | std::ios::ate);
      if (!ifs) {
        app.getLoop()->defer([res = task.res, ctx = task.file_context]() {
          auto ptr = ctx.lock();
          if (ptr && !ptr->aborted) {
            res->writeStatus("404 Not Found");
            phantomchat::security::writeHeaders(res, ptr->cors_origin);
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


      app.getLoop()->defer(
        [res = task.res, bytes, cors_origin = context_ptr->cors_origin, filename = context_ptr->filename]() {
          res->writeStatus("200 OK");
          phantomchat::security::writeHeaders(res, cors_origin);

          phantomchat::utils::FileProvider file_provider;
          res->writeHeader("Content-Type", file_provider.mimeType(filename));

          std::string just_filename = std::filesystem::path(filename).filename().string();

          res->writeHeader("Content-Disposition", "attachment; filename=\"" + just_filename + "\"");

          phantomchat::http::sendChunked(res, std::const_pointer_cast<const std::vector<char>>(bytes));
        });
    }
  };

  return std::jthread(std::move(worker_loop));
}


std::jthread eventLoggerBackgroundProcess(
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::EventLogTask> &task_queue)
{
  auto worker_loop = [&task_queue](std::stop_token stop) {
    std::unordered_map<std::string, std::ofstream> files_map{};

    while (!stop.stop_requested()) {
      phantomchat::processors::EventLogTask task;
      if (!task_queue.wait_dequeue_timed(task, std::chrono::milliseconds(1000))) continue;

      namespace fs = std::filesystem;
      namespace cfg = phantomchat::config;
      const fs::path storage_dir = fs::path(cfg::AppSettings::getInstance().upload_path) / task.room_name;

      if (task.delete_room_on_empty) {
        auto &file = files_map.at(task.room_name);
        file.close();
        files_map.erase(task.room_name);

        std::error_code ec;
        fs::remove_all(storage_dir, ec);// Remove all files in the room's upload directory

        continue;
      }

      const fs::path file_path = storage_dir / (task.room_name + ".ndjson");
      // file should be opened till the room is deleted, so we can just keep it open and append to it
      auto &file = files_map[task.room_name];
      if (!file.is_open() || !fs::exists(file_path)) {
        fs::create_directories(storage_dir);
        file.open(file_path, std::ios::out | std::ios::app);
      }
      if (!file) { continue; }
      file << task.json_event << '\n';
      file.flush();
    }
  };

  return std::jthread(std::move(worker_loop));
}

}// namespace phantomchat::processors

template std::jthread phantomchat::processors::uploadDocumentBackgroundProcess<uWS::App, false>(
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::UploadTask<uWS::App, false>> &,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::EventLogTask> &);

template std::jthread phantomchat::processors::downloadDocumentBackgroundProcess<uWS::App, false>(
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::DownloadTask<uWS::App, false>> &);

template std::jthread phantomchat::processors::uploadDocumentBackgroundProcess<uWS::SSLApp, true>(
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::UploadTask<uWS::SSLApp, true>> &,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::EventLogTask> &);

template std::jthread phantomchat::processors::downloadDocumentBackgroundProcess<uWS::SSLApp, true>(
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::DownloadTask<uWS::SSLApp, true>> &);
