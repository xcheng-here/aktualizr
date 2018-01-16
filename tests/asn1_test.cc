#include <gtest/gtest.h>

#include <string>

#include <ImageFile.h>

static int writeStream (const void *buffer, size_t size, void *arg) {
  std::string *out = static_cast<std::string*>(arg);
  *out += std::string(static_cast<const char*>(buffer), size);

  return 0;
};

TEST(asn1, serialize_simple) {
  ImageFile_t imgFile;
  memset(&imgFile, 0, sizeof(imgFile));

  OCTET_STRING_fromBuf(&imgFile.filename, "ex.img", -1);
  imgFile.numberOfBlocks = 1;
  imgFile.blockSize = 256;

  std::string out;

  //der_encode(&asn_DEF_ImageFile, static_cast<void*>(&imgFile), writeStream, &out);
  xer_encode(&asn_DEF_ImageFile, static_cast<void*>(&imgFile),
      XER_F_CANONICAL, writeStream, &out);

  EXPECT_EQ(out, "<ImageFile><filename>ex.img</filename><numberOfBlocks>1</numberOfBlocks><blockSize>256</blockSize></ImageFile>");

  OCTET_STRING_free(&asn_DEF_OCTET_STRING, &imgFile.filename, 1);
}

#ifndef __NO_MAIN__
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif
