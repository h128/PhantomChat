#pragma once

#include "../processors/DocumentFileUploadProcessor.h"
#include <App.h>
#include <algorithm>
#include <filesystem>
#include <memory>

#include <phantomchat/JsonMapper.hpp>
#include <phantomchat/contracts/PerSocketData.h>
#include <phantomchat/contracts/PhantomRequests.h>
#include <phantomchat/contracts/PhantomResponses.h>
#include <phantomchat/events/Events.h>
#include <phantomchat/services/RoomManager.h>

#define MAX_UPLOAD_SIZE_BYTES (100 * 1024 * 1024)// 100 MB

namespace phantomchat::handlers {
using namespace phantomchat::processors;
using namespace phantomchat::services;


template<typename ResponseType, typename RequestType>
void handleDocumentUpload(ResponseType *res,
  RequestType *req,
  RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<UploadTask> &taskQueue)
{

  // 1. Sanitize filename (remove paths)
  std::string rawName(req->getParameter("filename"));
  std::string safeName = std::filesystem::path(rawName).filename().string();

  // 2. Initial size check (if header exists)
  std::string_view length = req->getHeader("content-length");
  if (!length.empty()) {
    try {
      if (std::stoll(std::string(length)) > MAX_UPLOAD_SIZE_BYTES) {
        res->writeStatus("413 Payload Too Large")->end();
        return;
      }
    } catch (const std::exception &) {
      res->writeStatus("400 Bad Request")->end();
      return;
    }
  }

  // 3. Validation Check
  std::string roomName = std::string(req->getHeader("x-room-name"));
  std::string userId = std::string(req->getHeader("x-user-uuid"));

  if (roomName.empty() || userId.empty()) {
    res->writeStatus("400 Bad Request")->end("Headers missing x-room-name or x-user-uuid");
    return;
  }

  const auto roomOpt = room_manager.getRoom(roomName);
  if (!roomOpt
      || std::find(roomOpt->get().members.begin(), roomOpt->get().members.end(), userId)
           == roomOpt->get().members.end()) {
    res->writeStatus("403 Forbidden")->end("User is not a member of the specified room");
    return;
  }

  auto uploadPath = std::filesystem::path("storage") / roomName / safeName;
  std::filesystem::create_directories(uploadPath.parent_path());
  auto fileContext = std::make_shared<FileContext>(uploadPath.string());
  auto bytesReceived = std::make_shared<size_t>(0);

  res->onAborted([fileContext]() { fileContext->aborted = true; });
  res->onData([res, fileContext, bytesReceived, &taskQueue](std::string_view chunk, bool isLast) {
    *bytesReceived += chunk.size();

    // 4. Hard limit check
    if (*bytesReceived > MAX_UPLOAD_SIZE_BYTES) {
      res->close();// Immediate disconnect
      return;
    }

    taskQueue.enqueue({ .res = res,
      .fileData = std::vector<char>(chunk.begin(), chunk.end()),
      .fileContext = std::weak_ptr<FileContext>(fileContext),
      .isLastChunk = isLast });
  });
}
}// namespace phantomchat::handlers