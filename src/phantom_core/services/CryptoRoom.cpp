#include <array>
#include <phantomchat/services/CryptoRoom.h>
#include <sodium.h>
#include <stdexcept>
#include <vector>

namespace phantomchat::services::crypto_room {

void init()
{
  if (sodium_init() == -1) { throw std::runtime_error("Failed to initialize libsodium"); }
}

std::string generateRoomKey()
{
  constexpr int room_key_size = 32;

  std::array<unsigned char, room_key_size> key{};
  randombytes_buf(key.data(), key.size());

  std::array<char, room_key_size * 2 + 1> hex{};
  sodium_bin2hex(hex.data(), hex.size(), key.data(), key.size());

  return std::string(hex.data(), hex.size() - 1);
}

KeyPair genNewKeyPair()
{
  std::array<unsigned char, crypto_box_PUBLICKEYBYTES> pk{};
  std::array<unsigned char, crypto_box_SECRETKEYBYTES> sk{};
  crypto_box_keypair(pk.data(), sk.data());

  std::array<char, crypto_box_PUBLICKEYBYTES * 2 + 1> pk_hex{};
  sodium_bin2hex(pk_hex.data(), pk_hex.size(), pk.data(), pk.size());

  std::array<char, crypto_box_SECRETKEYBYTES * 2 + 1> sk_hex{};
  sodium_bin2hex(sk_hex.data(), sk_hex.size(), sk.data(), sk.size());

  return { .public_key = std::string(pk_hex.data(), pk_hex.size() - 1),
    .secret_key = std::string(sk_hex.data(), sk_hex.size() - 1) };
}

std::string encryptRoomKey(const std::string &room_key, const std::string &user_public_key_hex)
{
  // Decode user's hex public key
  std::array<unsigned char, crypto_box_PUBLICKEYBYTES> user_pk{};
  if (sodium_hex2bin(user_pk.data(),
        user_pk.size(),
        user_public_key_hex.c_str(),
        user_public_key_hex.size(),
        nullptr,
        nullptr,
        nullptr)
      != 0) {
    throw std::invalid_argument("Invalid user public key hex");
  }

  const auto *plaintext = reinterpret_cast<const unsigned char *>(room_key.data());
  const auto plaintext_len = room_key.size();

  // crypto_box_seal: ciphertext = crypto_box_SEALBYTES + plaintext_len
  const auto ciphertext_len = crypto_box_SEALBYTES + plaintext_len;
  std::vector<unsigned char> ciphertext(ciphertext_len);

  if (crypto_box_seal(ciphertext.data(), plaintext, plaintext_len, user_pk.data()) != 0) {
    throw std::runtime_error("Encryption failed");
  }

  // Return hex-encoded ciphertext
  std::vector<char> hex(ciphertext_len * 2 + 1);
  sodium_bin2hex(hex.data(), hex.size(), ciphertext.data(), ciphertext.size());

  return std::string(hex.data(), hex.size() - 1);
}

}// namespace phantomchat::services::crypto_room
