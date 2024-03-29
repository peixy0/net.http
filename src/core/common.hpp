#pragma once
#include <cstdint>
#include <string>

namespace common {

char ToChar(std::uint64_t);

void ToLower(std::string&);

std::string SHA1(std::string_view);

std::string Base64(std::string_view);

}  // namespace common
