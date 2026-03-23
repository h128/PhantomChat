#include "PhantomRequests.h"
#include <stdexcept>

namespace phantomchat::contracts {

void JoinOrCreateRoomRequest::validate() const
{
  if (user_uuid.empty()) { throw std::invalid_argument("user_uuid cannot be empty"); }
  if (public_key.empty()) { throw std::invalid_argument("public_key cannot be empty"); }
  if (room_name.empty()) { throw std::invalid_argument("room_name cannot be empty"); }
}

void SendMessageRequest::validate() const
{
  if (user_uuid.empty()) { throw std::invalid_argument("user_uuid cannot be empty"); }
  if (room_name.empty()) { throw std::invalid_argument("room_name cannot be empty"); }
  if (message.empty()) { throw std::invalid_argument("message cannot be empty"); }
}

}// namespace phantomchat::contracts