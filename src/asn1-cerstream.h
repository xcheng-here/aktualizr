#include "asn1-cer.h"

// tokens
namespace asn1 {

class Token 
{
  public:
    enum TokType {seq_tok, endseq_tok};
    Token(TokType t) {type = t;}
    TokType type;
};

const Token seq = Token(Token::seq_tok);
const Token endseq = Token(Token::endseq_tok);

class Serializer {
  public:
    const std::string& getResult() {return result;}

    Serializer& operator << (int32_t val);
    Serializer& operator << (std::string val);
    Serializer& operator << (ASN1_UniversalTag tag);
    Serializer& operator << (Token tok);
  private:
    std::string result;
    ASN1_UniversalTag last_type;
};

}
