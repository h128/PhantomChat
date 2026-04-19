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

template<bool SSL = false> struct UploadTask
{
  uWS::HttpResponse<SSL> *res = nullptr;
  std::vector<char> file_data;
  std::weak_ptr<FileContext> file_context;
  bool is_last_chunk = false;
};

template<bool SSL = false> struct DownloadTask
{
  uWS::HttpResponse<SSL> *res = nullptr;
  std::weak_ptr<FileContext> file_context;
};

struct EventLogTask
{
  std::string room_name{};
  std::string json_event{};
  bool delete_room_on_empty = false;
};


template<bool SSL>
std::jthread uploadDocumentBackgroundProcess(uWS::TemplatedApp<SSL> *app,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &event_logger,
  moodycamel::BlockingConcurrentQueue<UploadTask<SSL>> &task_queue);

template<bool SSL>
std::jthread downloadDocumentBackgroundProcess(uWS::TemplatedApp<SSL> *app,
  moodycamel::BlockingConcurrentQueue<DownloadTask<SSL>> &task_queue);

std::jthread eventLoggerBackgroundProcess(moodycamel::BlockingConcurrentQueue<EventLogTask> &task_queue);

}// namespace phantomchat::processors
