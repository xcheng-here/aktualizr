#include "asn1-cerstream.h"

namespace asn1 {

Serializer& Serializer::operator<<(int32_t val) {
  result.append(cer_encode_integer(val));
  return *this;
}

Serializer& Serializer::operator<<(std::string val) {
  result.append(cer_encode_string(val, last_type));
  return *this;
}

Serializer& Serializer::operator<<(ASN1_UniversalTag tag) {
  result.push_back(tag & 0xFF);  // only support primitive universal tags, sequence serialized with tokens
  last_type = tag;
  return *this;
}

Serializer& Serializer::operator<<(Token tok) {
  switch (tok.type) {
    case Token::seq_tok:
      result.push_back(0x30);
      result.push_back(0x80);
      break;
    case Token::endseq_tok:
      result.push_back(0x00);
      result.push_back(0x00);
      break;
    default:
      throw std::runtime_error("Unknown token type in ASN1 serialization");
  }
  return *this;
}
}
