#include <gtest/gtest.h>

#include <string>

#include "secondary_config.h"

TEST(asn1, serialize_simple) {
  SecondaryConfig conf = {
    secVirtual,
    true,
    "serial",
    "hwid",
    "clientdir",
    "privatekey",
    "publickey",
    "fwpath",
    "targetnamepath",
    "metadatapath",
  };
  BitStream bitStr;
  unsigned char buf[SecondaryConfig_REQUIRED_BYTES_FOR_ENCODING];
  int err;

  BitStream_Init(&bitStr, buf, sizeof(buf));

  SecondaryConfig_Encode(&conf, &bitStr, &err, 1);

  EXPECT_FALSE(err);

  BitStream_AttachBuffer(&bitStr, buf, sizeof(buf));
  memset(&conf, 0, sizeof(conf));

  SecondaryConfig_Decode(&conf, &bitStr, &err);

  EXPECT_FALSE(err);
}

#ifndef __NO_MAIN__
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#endif
