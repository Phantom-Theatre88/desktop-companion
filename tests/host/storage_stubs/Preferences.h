#pragma once
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
class Preferences {
 public:
  inline static std::map<std::string,std::vector<uint8_t>> bytes;
  inline static bool fail_open=false;
  inline static size_t write_limit=SIZE_MAX;
  bool begin(const char* name,bool readonly) { ns_=name;readonly_=readonly;return !fail_open; }
  void end() {}
  size_t getBytesLength(const char* key) { return bytes[ns_+":"+key].size(); }
  size_t getBytes(const char* key,void* out,size_t size) {
    const auto& b=bytes[ns_+":"+key];size=std::min(size,b.size());
    if(size) std::memcpy(out,b.data(),size);return size;
  }
  size_t putBytes(const char* key,const void* in,size_t size) {
    if(readonly_) return 0;
    size=std::min(size,write_limit);const auto* p=static_cast<const uint8_t*>(in);
    bytes[ns_+":"+key]=std::vector<uint8_t>(p,p+size);return size;
  }
  bool getBool(const char* key,bool fallback) { uint8_t v=0;return getBytes(key,&v,1)==1?v!=0:fallback; }
  float getFloat(const char* key,float fallback) { float v=0;return getBytes(key,&v,4)==4?v:fallback; }
  size_t putBool(const char* key,bool value) {uint8_t v=value?1:0;return putBytes(key,&v,1);}
  size_t putFloat(const char* key,float value) {return putBytes(key,&value,4);}
 private:
  std::string ns_; bool readonly_=true;
};
