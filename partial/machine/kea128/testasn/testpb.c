#include <string.h>

#include "SKEAZ1284.h" /* include peripheral declarations SKEAZ128M4 */
#include "pb_decode.h"
#include "pb_encode.h"
#include "secondary_config.pb.h"

static pb_byte_t outb[uptane_proto_SecondaryConfig_size];
static const pb_byte_t buf[] = {
  0x08, 0x02, 0x1a, 0x04, 0x61, 0x62, 0x63, 0x64, 0x20, 0x01, 0x2a, 0x0b,
  0x70, 0x72, 0x69, 0x76, 0x61, 0x74, 0x65, 0x2e, 0x6b, 0x65, 0x79, 0x32,
  0x0a, 0x70, 0x75, 0x62, 0x6c, 0x69, 0x63, 0x2e, 0x6b, 0x65, 0x79, 0x3a,
  0x0a, 0x63, 0x6c, 0x69, 0x65, 0x6e, 0x74, 0x5f, 0x64, 0x69, 0x72, 0x42,
  0x0d, 0x66, 0x69, 0x72, 0x6d, 0x77, 0x61, 0x72, 0x65, 0x5f, 0x70, 0x61,
  0x74, 0x68, 0x4a, 0x0b, 0x74, 0x61, 0x72, 0x67, 0x65, 0x74, 0x5f, 0x6e,
  0x61, 0x6d, 0x65, 0x52, 0x08, 0x6d, 0x65, 0x74, 0x61, 0x64, 0x61, 0x74,
  0x61,
};

void main(void) {
  uptane_proto_SecondaryConfig config = {
    .secondary_type = uptane_proto_SecondaryConfig_Type_UPTANE,
    .has_ecu_serial = false,
    .ecu_hardware_id = "abcd",
    .has_partial_verifying = true,
    .partial_verifying = true,
    .ecu_private_key = "private.key",
    .ecu_public_key = "public.key",
    .full_client_dir = "client_dir",
    .firmware_path = "firmware_path",
    .target_name_path = "target_name",
    .metadata_path = "metadata",
  };

  pb_ostream_t ostream = pb_ostream_from_buffer(outb, sizeof(outb));
  pb_encode(&ostream, uptane_proto_SecondaryConfig_fields, &config);

#if 0
  for (size_t k = 0; k < ostream.bytes_written; k++) {
    printf("0x%02x, ", outb[k]);
  }
  printf("\n");
#endif

  // decode
  pb_istream_t stream = pb_istream_from_buffer(buf, sizeof(buf));
  if (!pb_decode(&stream, uptane_proto_SecondaryConfig_fields, &config)) {
    //printf("decode error\n");
  }

  //printf("metadata_path: %s\n", config.metadata_path);

}
