#pragma once
#include <App.h>
#include <algorithm>
#include <atomic>
#include <blockingconcurrentqueue.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <phantomchat/JsonMapper.hpp>
#include <phantomchat/events/Events.h>
#include <phantomchat/utils/FileProvider.hpp>
#include <thread>

namespace phantomchat::processors {
struct FileContext
{
  std::string filename;
  std::string room_name;
  std::string user_uuid;

  std::fstream fileStream;
  bool is_poster = false;
  std::atomic<bool> aborted{ false };

  FileContext(const std::string &filename_,
    const std::string &room_name_,
    const std::string &user_uuid_,
    bool is_poster_)
    : filename(filename_), room_name(room_name_), user_uuid(user_uuid_), is_poster(is_poster_)
  {}
};
struct UploadTask
{
  uWS::HttpResponse<false> *res = nullptr;
  std::vector<char> fileData;
  std::weak_ptr<FileContext> fileContext;
  bool isLastChunk = false;
};

struct DownloadTask
{
  uWS::HttpResponse<false> *res = nullptr;
  std::weak_ptr<FileContext> fileContext;
};

inline std::atomic<bool> uploadProcessorRunning{ true };

template<typename APP_TYPE>
void uploadDocumentBackgroundProcess(APP_TYPE *app, moodycamel::BlockingConcurrentQueue<UploadTask> &taskQueue)
{
  auto workerLoop = [&taskQueue, app] {
    while (uploadProcessorRunning.load()) {
      UploadTask task;
      if (!taskQueue.wait_dequeue_timed(task, std::chrono::milliseconds(1000))) continue;

      auto contextPtr = task.fileContext.lock();
      if (!contextPtr || contextPtr->aborted) {
        // Context was destroyed or upload was aborted by client disconnect
        continue;
      }

      // Open file stream on first chunk
      if (!contextPtr->fileStream.is_open()) {
        contextPtr->fileStream.open(contextPtr->filename, std::ios::binary | std::ios::out);
        if (!contextPtr->fileStream) {
          // Failed to open file, close connection
          if (!contextPtr->aborted) { task.res->close(); }
          continue;
        }
      }

      // Write chunk to file
      contextPtr->fileStream.write(task.fileData.data(), static_cast<std::streamsize>(task.fileData.size()));

      if (!task.isLastChunk) continue;
      contextPtr->fileStream.close();

      if (contextPtr->aborted) continue;

      // Defer chunked response to the event loop
      app->getLoop()->defer([res = task.res] { res->writeStatus("200 OK")->end("File uploaded successfully"); });

      std::string justFilename = std::filesystem::path(contextPtr->filename).filename().string();

      phantomchat::events::FileUploadedEvent event(
        justFilename, contextPtr->room_name, contextPtr->user_uuid, contextPtr->is_poster);

      app->publish(event.room_name, json(event).dump(), uWS::OpCode::TEXT);
    }
  };

  std::jthread(workerLoop).detach();
}

template<typename APP_TYPE>
void downloadDocumentBackgroundProcess(APP_TYPE *app, moodycamel::BlockingConcurrentQueue<DownloadTask> &taskQueue)
{
  static constexpr std::size_t ChunkSizeBytes = 64U * 1024U;

  auto workerLoop = [&taskQueue, app] {
    while (uploadProcessorRunning.load()) {
      DownloadTask task;
      if (!taskQueue.wait_dequeue_timed(task, std::chrono::milliseconds(1000))) continue;

      auto contextPtr = task.fileContext.lock();
      if (!contextPtr || contextPtr->aborted) { continue; }

      // 1. Read file into buffer on background thread
      std::ifstream ifs(contextPtr->filename, std::ios::binary | std::ios::ate);
      if (!ifs) {
        app->getLoop()->defer([res = task.res, ctx = task.fileContext]() {
          auto ptr = ctx.lock();
          if (ptr && !ptr->aborted) { res->writeStatus("404 Not Found")->end("File not found"); }
        });
        continue;
      }

      auto fileSize = static_cast<std::size_t>(ifs.tellg());
      ifs.seekg(0, std::ios::beg);
      auto bytes = std::make_shared<std::vector<char>>(fileSize);
      ifs.read(bytes->data(), static_cast<std::streamsize>(fileSize));
      ifs.close();

      if (contextPtr->aborted) continue;

      std::string justFilename = std::filesystem::path(contextPtr->filename).filename().string();
      phantomchat::utils::FileProvider fileProvider;
      std::string contentType(fileProvider.mimeType(contextPtr->filename));

      // 2. Defer chunked response to the event loop
      app->getLoop()->defer([res = task.res, bytes, justFilename, contentType]() {
        res->writeStatus("200 OK");
        res->writeHeader("Content-Type", contentType);
        res->writeHeader("Content-Disposition", "attachment; filename=\"" + justFilename + "\"");
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
            const auto chunkSize = std::min<std::size_t>(ChunkSizeBytes, remaining);
            const auto ok = res->write(std::string_view(bytes->data() + *offset, chunkSize));
            *offset += chunkSize;
            if (!ok) { return true; }
          }
          *finished = true;
          res->end();
          return false;// Stop writable events once done
        });

        // Initial write attempt
        while (*offset < bytes->size()) {
          const auto remaining = bytes->size() - *offset;
          const auto chunkSize = std::min<std::size_t>(ChunkSizeBytes, remaining);
          const auto ok = res->write(std::string_view(bytes->data() + *offset, chunkSize));
          *offset += chunkSize;
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

  std::jthread(workerLoop).detach();
}
}// namespace phantomchat::processors