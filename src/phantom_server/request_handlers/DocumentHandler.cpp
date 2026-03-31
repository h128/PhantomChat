#include "../headers/DocumentHandler.h"

#include <filesystem>
#include <memory>
#include <phantomchat/utils/HelperFunctions.h>

#define MAX_UPLOAD_SIZE_BYTES (100 * 1024 * 1024)// 100 MB

namespace phantomchat::handlers {

using namespace phantomchat::processors;
using namespace phantomchat::services;
using namespace phantomchat::utils;

template<typename ResponseType, typename RequestType>
void handleUploadDocument(ResponseType *res,
  RequestType *req,
  RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<UploadTask> &task_queue)
{
  // 1. Sanitize filename (remove paths)
  std::string raw_name = url_decode(req->getParameter("filename"));
  std::string safe_file_name = std::filesystem::path(raw_name).filename().string();

  // 2. Initial size check (if header exists)
  auto content_length = str_to_long(req->getHeader("content-length"));
  if (content_length <= 0) {
    res->writeStatus("400 Bad Request")->end("Invalid Content-Length");
    return;
  }
  if (content_length > MAX_UPLOAD_SIZE_BYTES) {
    res->writeStatus("413 Payload Too Large")->end();
    return;
  }

  // 3. Validation Check
  std::string room_name = std::string(req->getHeader("x-room-name"));
  std::string user_id = std::string(req->getHeader("x-user-uuid"));

  if (room_name.empty() || user_id.empty()) {
    res->writeStatus("400 Bad Request")->end("Headers missing x-room-name or x-user-uuid");
    return;
  }

  if (!room_manager.isUserMemberOfRoom({ .room_name = room_name, .user_uuid = user_id })) {
    res->writeStatus("403 Forbidden")->end("User is not a member of the specified room");
    return;
  }

  // 4. Prepare for file upload
  auto upload_path = std::filesystem::path(UploadTask::upload_root_path) / room_name / safe_file_name;
  std::filesystem::create_directories(upload_path.parent_path());
  bool is_poster = safe_file_name.find("poster") != std::string::npos;
  auto file_context = std::make_shared<FileContext>(upload_path.string(), room_name, user_id, is_poster);
  auto bytes_received = std::make_shared<size_t>(0);

  res->onAborted([file_context]() { file_context->aborted = true; });
  res->onData([res, file_context, bytes_received, &task_queue, content_length](std::string_view chunk, bool is_last) {
    *bytes_received += chunk.size();

    // Hard limit check
    if (*bytes_received > content_length) {
      res->close();// Immediate disconnect
      return;
    }

    task_queue.enqueue({ .res = res,
      .file_data = std::vector<char>(chunk.begin(), chunk.end()),
      .file_context = std::weak_ptr<FileContext>(file_context),
      .is_last_chunk = is_last });
  });
}

template<typename ResponseType, typename RequestType>
void handleDownloadDocument(ResponseType *res,
  RequestType *req,
  moodycamel::BlockingConcurrentQueue<DownloadTask> &task_queue)
{
  // 1. Sanitize filename (remove paths)
  std::string raw_name = url_decode(req->getParameter("filename"));
  std::string safe_file_name = std::filesystem::path(raw_name).filename().string();

  if (safe_file_name.empty()) {
    res->writeStatus("400 Bad Request")->end("Missing filename");
    return;
  }

  // 2. Validation Check
  std::string room_name = std::string(req->getParameter("room"));
  std::string safe_room_name = std::filesystem::path(room_name).filename().string();
  if (safe_room_name.empty()) {
    res->writeStatus("400 Bad Request")->end("Missing room parameter");
    return;
  }

  // 3. Prepare for file download
  auto download_path = std::filesystem::path(UploadTask::upload_root_path) / safe_room_name / safe_file_name;
  bool is_poster = safe_file_name.find("poster") != std::string::npos;
  auto file_context = std::make_shared<FileContext>(download_path.string(), safe_room_name, "", is_poster);

  res->onAborted([file_context]() { file_context->aborted = true; });

  task_queue.enqueue({
    .res = res,
    .file_context = std::weak_ptr<FileContext>(file_context),
  });
}

}// namespace phantomchat::handlers

template void phantomchat::handlers::handleUploadDocument<uWS::HttpResponse<false>, uWS::HttpRequest>(
  uWS::HttpResponse<false> *,
  uWS::HttpRequest *,
  phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::UploadTask> &);

template void phantomchat::handlers::handleDownloadDocument<uWS::HttpResponse<false>, uWS::HttpRequest>(
  uWS::HttpResponse<false> *,
  uWS::HttpRequest *,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::DownloadTask> &);
