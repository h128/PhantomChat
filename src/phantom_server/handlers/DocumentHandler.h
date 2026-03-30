#pragma once

#include "../processors/DocumentProcessor.h"
#include <App.h>
#include <filesystem>
#include <memory>


#include <phantomchat/contracts/PerSocketData.h>
#include <phantomchat/contracts/PhantomRequests.h>
#include <phantomchat/contracts/PhantomResponses.h>
#include <phantomchat/events/Events.h>
#include <phantomchat/services/RoomManager.h>
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
  moodycamel::BlockingConcurrentQueue<UploadTask> &taskQueue)
{

  // 1. Sanitize filename (remove paths)
  std::string rawName = url_decode(req->getParameter("filename"));
  std::string safeFileName = std::filesystem::path(rawName).filename().string();

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
  std::string roomName = std::string(req->getHeader("x-room-name"));
  std::string userId = std::string(req->getHeader("x-user-uuid"));

  if (roomName.empty() || userId.empty()) {
    res->writeStatus("400 Bad Request")->end("Headers missing x-room-name or x-user-uuid");
    return;
  }

  if (!room_manager.isUserMemberOfRoom({ .room_name = roomName, .user_uuid = userId })) {
    res->writeStatus("403 Forbidden")->end("User is not a member of the specified room");
    return;
  }

  // 4. Prepare for file upload
  auto uploadPath = std::filesystem::path("storage") / roomName / safeFileName;
  std::filesystem::create_directories(uploadPath.parent_path());
  bool isPoster = safeFileName.find("poster") != std::string::npos;
  auto fileContext = std::make_shared<FileContext>(uploadPath.string(), roomName, userId, isPoster);
  auto bytesReceived = std::make_shared<size_t>(0);

  res->onAborted([fileContext]() { fileContext->aborted = true; });
  res->onData([res, fileContext, bytesReceived, &taskQueue, content_length](std::string_view chunk, bool isLast) {
    *bytesReceived += chunk.size();

    // 4. Hard limit check
    if (*bytesReceived > content_length) {
      res->close();// Immediate disconnect
      return;
    }

    taskQueue.enqueue({ .res = res,
      .fileData = std::vector<char>(chunk.begin(), chunk.end()),
      .fileContext = std::weak_ptr<FileContext>(fileContext),
      .isLastChunk = isLast });
  });
}

template<typename ResponseType, typename RequestType>
void handleDownloadDocument(ResponseType *res,
  RequestType *req,
  moodycamel::BlockingConcurrentQueue<DownloadTask> &taskQueue)
{

  // 1. Sanitize filename (remove paths)
  std::string rawName = url_decode(req->getParameter("filename"));
  std::string safeFileName = std::filesystem::path(rawName).filename().string();

  if (safeFileName.empty()) {
    res->writeStatus("400 Bad Request")->end("Missing filename");
    return;
  }

  // 2. Validation Check
  std::string roomName = std::string(req->getParameter("room"));
  std::string safeRoomName = std::filesystem::path(roomName).filename().string();
  if (safeRoomName.empty()) {
    res->writeStatus("400 Bad Request")->end("Missing room parameter");
    return;
  }

  // 3. Prepare for file download
  auto downloadPath = std::filesystem::path("storage") / safeRoomName / safeFileName;
  bool isPoster = safeFileName.find("poster") != std::string::npos;
  auto fileContext = std::make_shared<FileContext>(downloadPath.string(), safeRoomName, "", isPoster);

  res->onAborted([fileContext]() { fileContext->aborted = true; });

  taskQueue.enqueue({
    .res = res,
    .fileContext = std::weak_ptr<FileContext>(fileContext),
  });
}

}// namespace phantomchat::handlers