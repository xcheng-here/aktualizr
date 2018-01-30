#include <stdbool.h>
#include <string.h>

#include "SKEAZ1284.h" /* include peripheral declarations SKEAZ128M4 */
#include "asn1scc/secondary_config.h"

/* int __write_console; */
/* int __read_console; */
/* int __close_console; */
/* int __pformatter; */

void main(void) {
  SecondaryConfig conf = {
    legacy,
    true,
    "serial",
    "hwid",
    "clientdir",
    "publickey",
    "fwpath",
    "targetnamepath",
    "metadatapath",
  };
  BitStream bitStr;
  unsigned char buf[SecondaryType_REQUIRED_BITS_FOR_ENCODING];
  int err;

  BitStream_Init(&bitStr, buf, sizeof(buf));

  SecondaryConfig_Encode(&conf, &bitStr, &err, 1);

  BitStream_Init(&bitStr, buf, sizeof(buf));
  SecondaryConfig_Decode(&conf, &bitStr, &err);
}

