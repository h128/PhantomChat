#pragma once

#include "DocumentProcessor.h"
#include <App.h>
#include <phantomchat/services/RoomManager.h>

namespace phantomchat::handlers {

template<bool SSL>
void handleUploadDocument(uWS::HttpResponse<SSL> *res,
  uWS::HttpRequest *req,
  phantomchat::services::RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::UploadTask<SSL>> &task_queue);

template<bool SSL>
void handleDownloadDocument(uWS::HttpResponse<SSL> *res,
  uWS::HttpRequest *req,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::DownloadTask<SSL>> &task_queue);

}// namespace phantomchat::handlers