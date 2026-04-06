#pragma once

#include <phantomchat/phantom_core_export.hpp>
#include <string>

namespace phantomchat::services::crypto_room {

// Initialize the crypto library. Must be called before any other crypto_room function.
PHANTOM_CORE_EXPORT void init();

struct PHANTOM_CORE_EXPORT KeyPair
{
  std::string public_key;// hex-encoded
  std::string secret_key;// hex-encoded
};

// Generate a symmetric room key. Returns a hex-encoded 32-byte random key.
PHANTOM_CORE_EXPORT std::string generateRoomKey();

// Generate a new asymmetric keypair. Returns hex-encoded public and secret keys.
PHANTOM_CORE_EXPORT KeyPair genNewKeyPair();

// Encrypt a room key using the user's hex-encoded public key.
// Returns a hex-encoded sealed box (crypto_box_seal).
PHANTOM_CORE_EXPORT std::string encryptRoomKey(const std::string &room_key, const std::string &user_public_key_hex);

}// namespace phantomchat::services::crypto_room
