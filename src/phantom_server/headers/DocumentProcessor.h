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

template<typename APP_TYPE> struct FileContext
{
  APP_TYPE &app;
  std::string filename;
  std::string room_name;
  std::string user_uuid;
  std::string cors_origin;

  std::fstream file_stream;
  bool is_poster = false;
  std::atomic<bool> aborted{ false };

  FileContext(APP_TYPE &app_,
    std::string filename_,
    std::string room_name_,
    std::string user_uuid_,
    bool is_poster_,
    std::string cors_origin_ = {})
    : app(app_), filename(std::move(filename_)), room_name(std::move(room_name_)), user_uuid(std::move(user_uuid_)),
      cors_origin(std::move(cors_origin_)), is_poster(is_poster_)
  {}
};

template<typename APP_TYPE, bool SSL = false> struct UploadTask
{
  uWS::HttpResponse<SSL> *res = nullptr;
  std::vector<char> file_data;
  std::weak_ptr<FileContext<APP_TYPE>> file_context;
  bool is_last_chunk = false;
};

template<typename APP_TYPE, bool SSL = false> struct DownloadTask
{
  uWS::HttpResponse<SSL> *res = nullptr;
  std::weak_ptr<FileContext<APP_TYPE>> file_context;
};

struct EventLogTask
{
  std::string room_name{};
  std::string json_event{};
  bool delete_room_on_empty = false;
};


template<typename APP_TYPE, bool SSL>
std::jthread uploadDocumentBackgroundProcess(moodycamel::BlockingConcurrentQueue<UploadTask<APP_TYPE, SSL>> &task_queue,
  moodycamel::BlockingConcurrentQueue<EventLogTask> &event_logger);

template<typename APP_TYPE, bool SSL>
std::jthread downloadDocumentBackgroundProcess(
  moodycamel::BlockingConcurrentQueue<DownloadTask<APP_TYPE, SSL>> &task_queue);

std::jthread eventLoggerBackgroundProcess(moodycamel::BlockingConcurrentQueue<EventLogTask> &task_queue);

}// namespace phantomchat::processors
