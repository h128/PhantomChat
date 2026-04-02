#pragma once
#include <App.h>
#include <atomic>
#include <blockingconcurrentqueue.h>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace phantomchat::processors {
struct FileContext
{
  std::string filename;
  std::string room_name;
  std::string user_uuid;
  std::string cors_origin;

  std::fstream file_stream;
  bool is_poster = false;
  std::atomic<bool> aborted{ false };

  FileContext(std::string filename_,
    std::string room_name_,
    std::string user_uuid_,
    bool is_poster_,
    std::string cors_origin_ = {})
    : filename(std::move(filename_)), room_name(std::move(room_name_)), user_uuid(std::move(user_uuid_)),
      cors_origin(std::move(cors_origin_)), is_poster(is_poster_)
  {}
};

struct UploadTask
{

  uWS::HttpResponse<false> *res = nullptr;
  std::vector<char> file_data;
  std::weak_ptr<FileContext> file_context;
  bool is_last_chunk = false;

  inline static std::string upload_root_path{ "" };
};

struct DownloadTask
{
  uWS::HttpResponse<false> *res = nullptr;
  std::weak_ptr<FileContext> file_context;
};

static inline std::atomic<bool> uploadProcessorRunning{ true };

template<typename APP_TYPE>
std::jthread uploadDocumentBackgroundProcess(APP_TYPE *app,
  moodycamel::BlockingConcurrentQueue<UploadTask> &task_queue);

template<typename APP_TYPE>
std::jthread downloadDocumentBackgroundProcess(APP_TYPE *app,
  moodycamel::BlockingConcurrentQueue<DownloadTask> &task_queue);

}// namespace phantomchat::processors
