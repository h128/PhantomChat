#include <array>
#include <memory>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
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

std::string encryptRoomKey(const EncryptRoomArgs &args)
{
  // Decode user's hex public key
  std::array<unsigned char, crypto_box_PUBLICKEYBYTES> user_pk{};
  if (sodium_hex2bin(user_pk.data(),
        user_pk.size(),
        args.user_public_key_hex.c_str(),
        args.user_public_key_hex.size(),
        nullptr,
        nullptr,
        nullptr)
      != 0) {
    throw std::invalid_argument("Invalid user public key hex");
  }

  // Decode server's hex secret key
  std::array<unsigned char, crypto_box_SECRETKEYBYTES> server_sk{};
  if (sodium_hex2bin(server_sk.data(),
        server_sk.size(),
        args.server_key_pair.secret_key.c_str(),
        args.server_key_pair.secret_key.size(),
        nullptr,
        nullptr,
        nullptr)
      != 0) {
    throw std::invalid_argument("Invalid server secret key hex");
  }

  const auto *plaintext = reinterpret_cast<const unsigned char *>(args.room_key.data());
  const auto plaintext_len = args.room_key.size();

  // Use zero nonce (client derives the same nonce)
  std::array<unsigned char, crypto_box_NONCEBYTES> nonce{};

  // crypto_box_easy: ciphertext = crypto_box_MACBYTES + plaintext_len
  const auto ciphertext_len = crypto_box_MACBYTES + plaintext_len;
  std::vector<unsigned char> ciphertext(ciphertext_len);

  if (crypto_box_easy(ciphertext.data(), plaintext, plaintext_len, nonce.data(), user_pk.data(), server_sk.data())
      != 0) {
    throw std::runtime_error("Encryption failed");
  }

  // Return hex-encoded ciphertext
  std::vector<char> hex(ciphertext_len * 2 + 1);
  sodium_bin2hex(hex.data(), hex.size(), ciphertext.data(), ciphertext.size());

  return std::string(hex.data(), hex.size() - 1);
}


std::vector<unsigned char> rs256_sign(std::string_view privateKey, std::string_view message)
{

  auto load_private_key_from_pem = [](std::string_view pem) -> auto
  {
    constexpr auto bio_deleter = [](BIO *b) noexcept { BIO_free(b); };
    std::unique_ptr<BIO, decltype(bio_deleter)> bio{ BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size())) };
    if (!bio) throw std::runtime_error("BIO_new_mem_buf");

    constexpr auto pkey_deleter = [](EVP_PKEY *p) noexcept { EVP_PKEY_free(p); };
    std::unique_ptr<EVP_PKEY, decltype(pkey_deleter)> pkey{ PEM_read_bio_PrivateKey(
      bio.get(), nullptr, nullptr, nullptr) };
    if (!pkey) throw std::runtime_error("PEM_read_bio_PrivateKey");
    return pkey;
  };

  auto pkey = load_private_key_from_pem(privateKey);


  constexpr auto ctx_deleter = [](EVP_MD_CTX *c) noexcept { EVP_MD_CTX_free(c); };
  std::unique_ptr<EVP_MD_CTX, decltype(ctx_deleter)> ctx{ EVP_MD_CTX_new() };
  if (!ctx) throw std::runtime_error("EVP_MD_CTX_new");

  if (EVP_DigestSignInit(ctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) != 1)
    throw std::runtime_error("EVP_DigestSignInit");

  if (EVP_DigestSignUpdate(ctx.get(), message.data(), message.size()) != 1)
    throw std::runtime_error("EVP_DigestSignUpdate");

  std::size_t sig_len = 0;
  if (EVP_DigestSignFinal(ctx.get(), nullptr, &sig_len) != 1)
    throw std::runtime_error("EVP_DigestSignFinal (size query)");

  std::vector<unsigned char> sig(sig_len);
  if (EVP_DigestSignFinal(ctx.get(), sig.data(), &sig_len) != 1) throw std::runtime_error("EVP_DigestSignFinal");
  sig.resize(sig_len);
  return sig;
}


}// namespace phantomchat::services::crypto_room
