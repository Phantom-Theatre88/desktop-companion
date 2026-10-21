#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
constexpr int CARD_NONE=0, FILE_READ=0, FILE_WRITE=1;
class TestFile {
 public:
  std::vector<uint8_t>* data=nullptr;
  size_t limit=SIZE_MAX;
  explicit operator bool() const { return data!=nullptr; }
  size_t size() const {return data?data->size():0;}
  size_t read(uint8_t* out,size_t n) {n=std::min(n,size());if(n)std::memcpy(out,data->data(),n);return n;}
  size_t write(const uint8_t* p,size_t n) {n=std::min(n,limit);data->insert(data->end(),p,p+n);return n;}
  void flush() {}
  void close() {}
};
class TestSD {
 public:
  int card=0; size_t write_limit=SIZE_MAX;
  std::map<std::string,std::vector<uint8_t>> files;
  int cardType() const {return card;}
  bool exists(const char* p) const {return files.count(p)!=0;}
  bool remove(const char* p) {return files.erase(p)==1;}
  TestFile open(const char* p,int mode) {
    if(!card || (!exists(p)&&mode==FILE_READ))return {};
    return {&files[p],write_limit};
  }
};
inline TestSD SD;
