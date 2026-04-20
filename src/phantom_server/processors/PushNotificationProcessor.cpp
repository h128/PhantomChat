#include "../headers/PushNotificationProcessor.h"
#include <fmt/core.h>
#include <phantomchat/services/Firebase.h>

namespace phantomchat::processors {

std::jthread pushNotificationBackgroundProcess(moodycamel::BlockingConcurrentQueue<PushNotificationTask> &task_queue,
  services::RoomManager &room_manager,
  const config::AppSettings &settings)
{
  auto worker_loop = [&task_queue, &room_manager, &settings](std::stop_token stop) {
    const auto &fb = settings.firebase_settings;
    auto access_token = services::firebase::fetch_access_token(fb).token;

    while (!stop.stop_requested()) {
      PushNotificationTask task;
      if (!task_queue.wait_dequeue_timed(task, std::chrono::milliseconds(1000))) continue;
      if (task.recipients.empty()) continue;

      try {
        bool token_refreshed = false;

        for (const auto &fcm_token : task.recipients) {
          auto result = services::firebase::send_fcm_message(
            access_token, fb.project_id, fcm_token, task.title, task.body, task.icon);

          if (result == services::firebase::FcmSendResult::Unauthorized && !token_refreshed) {
            access_token = services::firebase::fetch_access_token(fb).token;
            token_refreshed = true;
            result = services::firebase::send_fcm_message(
              access_token, fb.project_id, fcm_token, task.title, task.body, task.icon);
          }

          if (result == services::firebase::FcmSendResult::Success) {
            room_manager.markPushed(task.room_name, fcm_token);
          }
        }
      } catch (const std::exception &e) {
        fmt::print(stderr, "[push] exception while processing task: {}\n", e.what());
      }
    }
  };

  return std::jthread(std::move(worker_loop));
}

}// namespace phantomchat::processors
