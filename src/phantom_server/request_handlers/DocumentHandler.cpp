#include "../headers/DocumentHandler.h"
#include "../headers/SecurityHeaders.h"

#include <filesystem>
#include <memory>
#include <phantomchat/config/AppSettings.h>
#include <phantomchat/utils/HelperFunctions.h>

static constexpr std::size_t max_upload_size_bytes = 100ULL * 1024 * 1024;

namespace phantomchat::handlers {

using namespace phantomchat::processors;
using namespace phantomchat::services;
using namespace phantomchat::utils;

template<typename APP_TYPE, bool SSL>
void handleUploadDocument(APP_TYPE &app,
  uWS::HttpResponse<SSL> *res,
  uWS::HttpRequest *req,
  const RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<UploadTask<APP_TYPE, SSL>> &task_queue)
{
  // 1. Sanitize filename (remove paths)
  std::string raw_name = url_decode(req->getParameter("filename"));
  std::string safe_file_name = std::filesystem::path(raw_name).filename().string();

  // 2. Initial size check (if header exists)
  auto content_length = str_to_long(req->getHeader("content-length"));
  if (content_length == 0) {
    res->writeStatus("400 Bad Request");
    phantomchat::security::writeHeaders(res, req);
    res->end("Invalid Content-Length");
    return;
  }
  if (content_length > max_upload_size_bytes) {
    res->writeStatus("413 Payload Too Large");
    phantomchat::security::writeHeaders(res, req);
    res->end();
    return;
  }

  // 3. Validation Check
  std::string room_name = std::string(req->getHeader("x-room-name"));
  std::string user_id = std::string(req->getHeader("x-user-uuid"));

  if (room_name.empty() || user_id.empty()) {
    res->writeStatus("400 Bad Request");
    phantomchat::security::writeHeaders(res, req);
    res->end("Headers missing x-room-name or x-user-uuid");
    return;
  }

  if (!room_manager.isUserMemberOfRoom({ .room_name = room_name, .user_uuid = user_id })) {
    res->writeStatus("403 Forbidden");
    phantomchat::security::writeHeaders(res, req);
    res->end("User is not a member of the specified room");
    return;
  }

  // 4. Prepare for file upload
  auto upload_path =
    std::filesystem::path(phantomchat::config::AppSettings::getInstance().upload_path) / room_name / safe_file_name;
  std::filesystem::create_directories(upload_path.parent_path());
  bool is_poster = safe_file_name.find("poster") != std::string::npos;
  auto resolved_origin = phantomchat::security::resolveOrigin(req->getHeader("origin"));
  auto file_context = std::make_shared<FileContext<APP_TYPE>>(
    app, upload_path.string(), std::move(room_name), std::move(user_id), is_poster, std::move(resolved_origin));
  auto bytes_received = std::make_shared<size_t>(0);

  res->onAborted([file_context]() { file_context->aborted = true; });
  res->onData(
    [&app, res, file_context, bytes_received, &task_queue, content_length](std::string_view chunk, bool is_last) {
      *bytes_received += chunk.size();

      // Hard limit check
      if (*bytes_received > content_length) {
        res->close();// Immediate disconnect
        return;
      }

      task_queue.enqueue({ .res = res,
        .file_data = std::vector<char>(chunk.begin(), chunk.end()),
        .file_context = std::weak_ptr<FileContext<APP_TYPE>>(file_context),
        .is_last_chunk = is_last });
    });
}

template<typename APP_TYPE, bool SSL>
void handleDownloadDocument(APP_TYPE &app,
  uWS::HttpResponse<SSL> *res,
  uWS::HttpRequest *req,
  const RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<DownloadTask<APP_TYPE, SSL>> &task_queue)
{
  // 1. Sanitize filename (remove paths)
  std::string raw_name = url_decode(req->getParameter("filename"));
  std::string safe_file_name = std::filesystem::path(raw_name).filename().string();

  if (safe_file_name.empty()) {
    res->writeStatus("400 Bad Request");
    phantomchat::security::writeHeaders(res, req);
    res->end("Missing filename");
    return;
  }

  // 2. Validation Check
  std::string room_name = std::string(req->getParameter("room"));
  std::string safe_room_name = std::filesystem::path(room_name).filename().string();
  if (safe_room_name.empty()) {
    res->writeStatus("400 Bad Request");
    phantomchat::security::writeHeaders(res, req);
    res->end("Missing room parameter");
    return;
  }

  std::string user_uuid = std::string(req->getHeader("x-user-uuid"));
  if (user_uuid.empty() || !room_manager.isUserMemberOfRoom({ .room_name = safe_room_name, .user_uuid = user_uuid })) {
    res->writeStatus("403 Forbidden");
    phantomchat::security::writeHeaders(res, req);
    res->end("User is not a member of the specified room");
    return;
  }

  // 3. Prepare for file download
  auto download_path = std::filesystem::path(phantomchat::config::AppSettings::getInstance().upload_path)
                       / safe_room_name / safe_file_name;
  bool is_poster = safe_file_name.find("poster") != std::string::npos;
  auto resolved_origin = phantomchat::security::resolveOrigin(req->getHeader("origin"));
  auto file_context = std::make_shared<FileContext<APP_TYPE>>(
    app, download_path.string(), std::move(safe_room_name), "", is_poster, std::move(resolved_origin));

  res->onAborted([file_context]() { file_context->aborted = true; });

  task_queue.enqueue({
    .res = res,
    .file_context = std::weak_ptr<FileContext<APP_TYPE>>(file_context),
  });
}

}// namespace phantomchat::handlers

template void phantomchat::handlers::handleUploadDocument<uWS::App, false>(uWS::App &,
  uWS::HttpResponse<false> *,
  uWS::HttpRequest *,
  const phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::UploadTask<uWS::App, false>> &);

template void phantomchat::handlers::handleDownloadDocument<uWS::App, false>(uWS::App &,
  uWS::HttpResponse<false> *,
  uWS::HttpRequest *,
  const phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::DownloadTask<uWS::App, false>> &);

template void phantomchat::handlers::handleUploadDocument<uWS::SSLApp, true>(uWS::SSLApp &,
  uWS::HttpResponse<true> *,
  uWS::HttpRequest *,
  const phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::UploadTask<uWS::SSLApp, true>> &);

template void phantomchat::handlers::handleDownloadDocument<uWS::SSLApp, true>(uWS::SSLApp &,
  uWS::HttpResponse<true> *,
  uWS::HttpRequest *,
  const phantomchat::services::RoomManager &,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::DownloadTask<uWS::SSLApp, true>> &);
