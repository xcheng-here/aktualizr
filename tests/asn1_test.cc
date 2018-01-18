#include <gtest/gtest.h>

#include <string>

#include <ImageFile.h>
#include "asn1/asn1_serialize.h"


TEST(asn1, serialize_simple) {
  ImageFile_t *imgFile = static_cast<ImageFile*>(calloc(1, sizeof(*imgFile)));

  OCTET_STRING_fromString(&imgFile->filename, "ex.img");
  imgFile->numberOfBlocks = 1;
  imgFile->blockSize = 256;

  EXPECT_EQ(asn1::serialize_xer(static_cast<void*>(imgFile), &asn_DEF_ImageFile),
      "<ImageFile><filename>ex.img</filename><numberOfBlocks>1</numberOfBlocks><blockSize>256</blockSize></ImageFile>");

  ASN_STRUCT_FREE(asn_DEF_ImageFile, imgFile);
}

#ifndef __NO_MAIN__
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif
