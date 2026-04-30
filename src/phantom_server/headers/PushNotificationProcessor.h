#pragma once

#include <blockingconcurrentqueue.h>
#include <phantomchat/config/AppSettings.h>
#include <phantomchat/services/RoomManager.h>
#include <string>
#include <thread>
#include <vector>

namespace phantomchat::processors {

struct PushNotificationTask
{
  std::string room_name;
  std::vector<std::string> recipients;
  std::string title;
  std::string body;
};

std::jthread pushNotificationBackgroundProcess(moodycamel::BlockingConcurrentQueue<PushNotificationTask> &task_queue,
  services::RoomManager &room_manager,
  const config::AppSettings &settings);

}// namespace phantomchat::processors
