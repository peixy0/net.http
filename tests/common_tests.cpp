#include <gtest/gtest.h>
#include "common.hpp"

using namespace testing;

namespace common {

TEST(CommonFunctionTest, whenEncodingWebsocketAcceptKey_itShouldProduceCorrectResult) {
  ASSERT_EQ(Base64(SHA1("x3JJHMbDL1EzLkh9GBhXDw=="
                        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")),
      "HSmrc0sMlYUkAGmm5OPpG2HaGWk=");
}

}  // namespace common
