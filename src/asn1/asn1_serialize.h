#ifndef ASN1_SERIALIZE_H_
#define ASN1_SERIALIZE_H_

#include <string>

namespace asn1 {

static inline int asn1_write_stream (const void *buffer, size_t size, void *arg) {
  std::string *out = static_cast<std::string*>(arg);
  *out += std::string(static_cast<const char*>(buffer), size);

  return 0;
};

std::string serialize_xer(void *v, asn_TYPE_descriptor_t *descr) {
  std::string out;

  xer_encode(descr, static_cast<void*>(v), XER_F_CANONICAL, asn1_write_stream, &out);
  return out;
}

}

#endif
