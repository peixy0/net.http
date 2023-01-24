#include "common.hpp"
#include <openssl/sha.h>

namespace common {

void ToLower(std::string& s) {
  for (char& c : s) {
    c = tolower(c);
  }
}

std::string SHA1(std::string_view payload) {
  char r[SHA_DIGEST_LENGTH];
  ::SHA1(reinterpret_cast<const unsigned char*>(payload.data()), payload.size(), reinterpret_cast<unsigned char*>(r));
  return std::string{r, r + SHA_DIGEST_LENGTH};
}

std::string Base64(std::string_view payload) {
  const char* alpha =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";
  int len = payload.size();
  const unsigned char* p = reinterpret_cast<const unsigned char*>(payload.data());
  std::string result;
  while (len >= 3) {
    std::uint32_t d = p[0] << 16 | p[1] << 8 | p[2];
    result += alpha[d >> 18 & 0b111111];
    result += alpha[d >> 12 & 0b111111];
    result += alpha[d >> 6 & 0b111111];
    result += alpha[d & 0b111111];
    p += 3;
    len -= 3;
  }
  if (len > 1) {
    std::uint32_t d = p[0] << 16 | p[1] << 8;
    result += alpha[d >> 18 & 0b111111];
    result += alpha[d >> 12 & 0b111111];
    result += alpha[d >> 6 & 0b111111];
    result += '=';
    return result;
  }
  if (len > 0) {
    std::uint32_t d = p[0] << 16;
    result += alpha[d >> 18 & 0b111111];
    result += alpha[d >> 12 & 0b111111];
    result += "==";
    return result;
  }
  return result;
}

}  // namespace common
