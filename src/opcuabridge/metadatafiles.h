#ifndef OPCUABRIDGE_METADATAFILES_H_
#define OPCUABRIDGE_METADATAFILES_H_

#include "common.h"

namespace opcuabridge {
class MetadataFiles {
 public:
  MetadataFiles() {}
  virtual ~MetadataFiles() {}
 public:
  const int& get_GUID() const { return GUID_; }
  void set_GUID(const int& GUID) { GUID_ = GUID; }
  const std::size_t& get_numberOfMetadataFiles() const { return numberOfMetadataFiles_; }
  void set_numberOfMetadataFiles(const std::size_t& numberOfMetadataFiles) {
    numberOfMetadataFiles_ = numberOfMetadataFiles;
  }
  INITSERVERNODESET_FUNCTION_DEFINITION(MetadataFiles)  // InitServerNodeset(UA_Server*)
  CLIENTREAD_FUNCTION_DEFINITION()                      // ClientRead(UA_Client*)
  CLIENTWRITE_FUNCTION_DEFINITION()                     // ClientWrite(UA_Client*)
 protected:
  int GUID_;
  std::size_t numberOfMetadataFiles_;
 private:
  static const char* node_id_;
 private:
  Json::Value wrapMessage() const {
    Json::Value v;
    v["GUID"] = get_GUID();
    v["numberOfMetadataFiles"] = static_cast<Json::Value::UInt>(get_numberOfMetadataFiles());
    return v;
  }
  void unwrapMessage(Json::Value v) {
    set_GUID(v["GUID"].asInt());
    set_numberOfMetadataFiles(v["numberOfMetadataFiles"].asUInt());
  }
 private:
  WRAPMESSAGE_FUCTION_DEFINITION(MetadataFiles)
  UNWRAPMESSAGE_FUCTION_DEFINITION(MetadataFiles)
  READ_FUNCTION_FRIEND_DECLARATION(MetadataFiles)
  WRITE_FUNCTION_FRIEND_DECLARATION(MetadataFiles)
  INTERNAL_FUNCTIONS_FRIEND_DECLARATION(MetadataFiles)
 private:
  #ifdef OPCUABRIDGE_ENABLE_SERIALIZATION
  SERIALIZE_FUNCTION_FRIEND_DECLARATION

  DEFINE_SERIALIZE_METHOD() {
    SERIALIZE_FIELD(ar, "GUID_", GUID_);
    SERIALIZE_FIELD(ar, "numberOfMetadataFiles_", numberOfMetadataFiles_);
  }
  #endif  // OPCUABRIDGE_ENABLE_SERIALIZATION
};
}  // namespace opcuabridge

#endif  // OPCUABRIDGE_METADATAFILES_H_
