#pragma once
#include <App.h>
#include <atomic>
#include <blockingconcurrentqueue.h>
#include <chrono>
#include <fstream>
#include <memory>
#include <thread>

namespace phantomchat::processors {
struct FileContext
{
  std::string filename;
  std::ofstream fileStream;
  std::atomic<bool> aborted{ false };

  FileContext(std::string filename_) : filename(std::move(filename_)) {}
};
struct UploadTask
{
  uWS::HttpResponse<false> *res = nullptr;
  std::vector<char> fileData;
  std::weak_ptr<FileContext> fileContext;
  bool isLastChunk = false;
};
inline std::atomic<bool> uploadProcessorRunning{ true };
void uploadDocumentBackgroundProcess(moodycamel::BlockingConcurrentQueue<UploadTask> &taskQueue)
{
  auto workerLoop = [&taskQueue] {
    while (uploadProcessorRunning.load()) {
      UploadTask task;
      if (taskQueue.wait_dequeue_timed(task, std::chrono::milliseconds(100))) {
        auto contextPtr = task.fileContext.lock();
        if (!contextPtr || contextPtr->aborted) {
          // Context was destroyed or upload was aborted by client disconnect
          continue;
        }

        // Open file stream on first chunk
        if (!contextPtr->fileStream.is_open()) {
          contextPtr->fileStream.open(contextPtr->filename, std::ios::binary);
          if (!contextPtr->fileStream) {
            // Failed to open file, close connection
            if (!contextPtr->aborted) { task.res->close(); }
            continue;
          }
        }

        // Write chunk to file
        contextPtr->fileStream.write(task.fileData.data(), static_cast<std::streamsize>(task.fileData.size()));

        if (task.isLastChunk) {
          contextPtr->fileStream.close();
          if (!contextPtr->aborted) { task.res->writeStatus("200 OK")->end("File uploaded successfully"); }
        }
      }
    }
  };

  std::jthread(workerLoop).detach();
}
}// namespace phantomchat::processors