#include <gtest/gtest.h>

#include <string>

#include <ECUModule.h>

TEST(asn1, serialize_simple) {
  ImageFile imgFile = {
    "ex.img", 1, 256,
  };
  BitStream bitStr;
  unsigned char buf[ImageFile_REQUIRED_BYTES_FOR_ENCODING];
  int err;

  BitStream_Init(&bitStr, buf, ImageFile_REQUIRED_BYTES_FOR_ENCODING);

  ImageFile_Encode(&imgFile, &bitStr, &err, 1);

  EXPECT_FALSE(err);
}

#ifndef __NO_MAIN__
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif
