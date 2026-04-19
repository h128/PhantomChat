#pragma once

#include "DocumentProcessor.h"
#include <App.h>
#include <phantomchat/services/RoomManager.h>

namespace phantomchat::handlers {

template<typename APP_TYPE, bool SSL>
void handleUploadDocument(APP_TYPE &app,
  uWS::HttpResponse<SSL> *res,
  uWS::HttpRequest *req,
  phantomchat::services::RoomManager &room_manager,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::UploadTask<APP_TYPE, SSL>> &task_queue);

template<typename APP_TYPE, bool SSL>
void handleDownloadDocument(APP_TYPE &app,
  uWS::HttpResponse<SSL> *res,
  uWS::HttpRequest *req,
  moodycamel::BlockingConcurrentQueue<phantomchat::processors::DownloadTask<APP_TYPE, SSL>> &task_queue);

}// namespace phantomchat::handlers