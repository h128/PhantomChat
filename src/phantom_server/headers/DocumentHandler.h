#pragma once

#include "DocumentProcessor.h"
#include <App.h>
#include <phantomchat/services/RoomManager.h>

namespace phantomchat::handlers {

template<typename ResponseType, typename RequestType>
void handleUploadDocument(ResponseType *res,
  RequestType *req,
  phantomchat::services::RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::UploadTask> &task_queue);

template<typename ResponseType, typename RequestType>
void handleDownloadDocument(ResponseType *res,
  RequestType *req,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::DownloadTask> &task_queue);

}// namespace phantomchat::handlers