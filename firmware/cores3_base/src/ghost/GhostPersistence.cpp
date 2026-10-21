#include "GhostPersistence.h"
#include <cmath>
#include <cstring>
#include <limits>
#include <initializer_list>
namespace deskbot { namespace ghost {
namespace {
uint32_t crc32(const uint8_t* bytes, size_t size) {
  uint32_t crc = 0xffffffffu;
  for (size_t i=0;i<size;++i) {
    crc ^= bytes[i];
    for (int b=0;b<8;++b) crc = (crc >> 1) ^ (0xedb88320u & (0u-(crc&1u)));
  }
  return ~crc;
}
struct Archive {
  uint8_t* bytes; size_t capacity, offset=0; bool reading, ok=true;
  void u32(uint32_t& value) {
    if (offset+4>capacity) { ok=false; return; }
    if (reading) value=0;
    for(unsigned i=0;i<4;++i) {
      if(reading) value |= static_cast<uint32_t>(bytes[offset++]) << (i*8);
      else bytes[offset++] = static_cast<uint8_t>(value >> (i*8));
    }
  }
  void u64(uint64_t& value) {
    uint32_t low=static_cast<uint32_t>(value), high=static_cast<uint32_t>(value>>32);
    u32(low);u32(high);if(reading) value=low | (static_cast<uint64_t>(high)<<32);
  }
  void number(float& value) {
    static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559, "checkpoint requires IEEE binary32");
    uint32_t bits=0; std::memcpy(&bits,&value,4);u32(bits);
    if(reading) std::memcpy(&value,&bits,4);
    if(!std::isfinite(value)) ok=false;
  }
  void boolean(bool& value) {
    uint32_t v=value?1:0;u32(v);if(v>1) ok=false;if(reading) value=v==1;
  }
  template<typename T> void enumeration(T& value, uint32_t max) {
    uint32_t v=static_cast<uint32_t>(value);u32(v);
    if(v>max) ok=false;if(reading) value=static_cast<T>(v);
  }
  void state(HeartState& s) {
    for(float* v : {&s.mood,&s.affection,&s.curiosity,&s.boredom,&s.sleepiness,&s.attention}) {
      number(*v);if(*v<0 || *v>1) ok=false;
    }
  }
  void context(HeartContext& c) { state(c.state);state(c.baseline);u32(c.captured_ms); }
  void evidence(HeartEventEvidence& e) {
    u32(e.person_id);enumeration(e.type,static_cast<uint32_t>(nerve::NeuronType::DARKER));
    boolean(e.has_prior);number(e.prior_mood_delta);number(e.prior_affection_delta);
    if(e.relationship_experiences>8 || e.positive_experiences>8 ||
       e.negative_experiences>8 || std::fabs(e.prior_mood_delta)>1 ||
       std::fabs(e.prior_affection_delta)>1) ok=false;
    uint32_t count=static_cast<uint32_t>(e.relationship_experiences);
    uint32_t pos=static_cast<uint32_t>(e.positive_experiences);
    uint32_t neg=static_cast<uint32_t>(e.negative_experiences);
    u32(count);u32(pos);u32(neg);
    if(count>8 || pos>count || neg>count) ok=false;
    if(reading) {e.relationship_experiences=count;e.positive_experiences=pos;e.negative_experiences=neg;}
  }
  void record(MemoryRecord& r) {
    enumeration(r.kind,static_cast<uint32_t>(MemoryRecordKind::REFLEX_RESULT));
    enumeration(r.neuron.type,static_cast<uint32_t>(nerve::NeuronType::DARKER));
    enumeration(r.neuron.source,static_cast<uint32_t>(nerve::NeuronSource::GHOST));
    u32(r.neuron.timestamp_ms);number(r.neuron.confidence);number(r.neuron.payload.scalar);
    uint32_t x=0,y=0;
    std::memcpy(&x,&r.neuron.payload.x,4);std::memcpy(&y,&r.neuron.payload.y,4);
    u32(x);u32(y);
    if(reading) {std::memcpy(&r.neuron.payload.x,&x,4);std::memcpy(&r.neuron.payload.y,&y,4);}
    u32(r.neuron.person_id);context(r.heart_context);context(r.outcome_context);
    boolean(r.has_outcome);evidence(r.appraisal_evidence);
    boolean(r.similar_prior_event);u32(r.occurred_ms);u32(r.session_id);
    if(r.kind!=MemoryRecordKind::SEMANTIC_EVENT || !r.has_outcome ||
       r.neuron.confidence<0 || r.neuron.confidence>1 || !r.session_id) ok=false;
  }
  void checkpoint(GhostCheckpoint& s) {
    uint32_t magic=0x36545347u,version=1;u32(magic);u32(version);
    if(magic!=0x36545347u || version!=1) ok=false;
    u64(s.generation);u64(s.unix_seconds);u32(s.quiet_seconds);state(s.heart);state(s.baseline);
    if(!s.generation || (s.unix_seconds &&
        (s.unix_seconds<946684800ULL || s.unix_seconds>4102444800ULL))) ok=false;
    u32(s.memory.session_id);u32(s.memory.count);
    if(!s.memory.session_id || s.memory.session_id==UINT32_MAX || s.memory.count>8) {ok=false;return;}
    for(size_t i=0;i<s.memory.count;++i) {
      record(s.memory.records[i]);
      if(s.memory.records[i].session_id>s.memory.session_id) ok=false;
    }
  }
};
}
size_t GhostPersistence::encode(const GhostCheckpoint& snapshot, uint8_t* data,size_t capacity) {
  if(!data || capacity<4) return 0;
  // Serialize fields, never struct padding, pointers, size_t or native bool ABI.
  auto copy=snapshot;
  Archive archive{data,capacity-4,0,false,true};archive.checkpoint(copy);
  if(!archive.ok) return 0;
  uint32_t crc=crc32(data,archive.offset);
  archive.capacity=capacity;archive.u32(crc);
  return archive.ok?archive.offset:0;
}
bool GhostPersistence::decode(const uint8_t* data,size_t size,GhostCheckpoint& out) {
  if(!data || size<4 || size>kMaxBytes) return false;
  uint32_t stored=0;
  for(unsigned i=0;i<4;++i) stored |= static_cast<uint32_t>(data[size-4+i])<<(8*i);
  if(stored!=crc32(data,size-4)) return false;
  GhostCheckpoint candidate;
  Archive archive{const_cast<uint8_t*>(data),size-4,0,true,true};archive.checkpoint(candidate);
  if(!archive.ok || archive.offset!=size-4) return false;
  out=candidate;return true;
}
bool GhostPersistence::loadStore(const CheckpointStore& store,GhostCheckpoint& out) {
  bool found=false;
  if(!store.read) return false;
  for(unsigned slot=0;slot<2;++slot) {
    const size_t size=store.read(slot,buffer_,kMaxBytes,store.context);
    GhostCheckpoint candidate;
    if(decode(buffer_,size,candidate) && (!found || candidate.generation>out.generation)) {
      out=candidate;found=true;
    }
  }
  return found;
}
bool GhostPersistence::load(GhostCheckpoint& out) {
  source_=CheckpointSource::NONE;generation_=0;
  if(loadStore(primary_,out)) source_=CheckpointSource::PRIMARY;
  else if(loadStore(backup_,out)) source_=CheckpointSource::BACKUP;
  else return false;
  generation_=out.generation;return true;
}
bool GhostPersistence::writeStore(const CheckpointStore& store,unsigned slot,
    const uint8_t* data,size_t size) {
  if(!store.write || !store.read || !store.write(slot,data,size,store.context)) return false;
  const size_t read=store.read(slot,verification_,kMaxBytes,store.context);
  // Exact read-back matches the already validated/versioned/CRC-encoded bytes.
  return read==size && std::memcmp(data,verification_,size)==0;
}
bool GhostPersistence::save(const GhostCheckpoint& input) {
  saving_=input;
  auto& snapshot=saving_;
  primary_save_ok_=backup_save_ok_=false;
  if(generation_==std::numeric_limits<uint64_t>::max()) return false;
  snapshot.generation=generation_+1;
  const size_t size=encode(snapshot,buffer_,kMaxBytes);
  if(!size) return false;
  const unsigned slot=static_cast<unsigned>(snapshot.generation%2);
  primary_save_ok_=writeStore(primary_,slot,buffer_,size);
  // A failed primary must not destroy the independent recovery checkpoint.
  if(primary_save_ok_) {
    generation_=snapshot.generation;
    // Backup may have missed generations. Alternate from its own last valid
    // slot, not primary parity, so a failed backup never replaces its sole copy.
    unsigned backup_slot=0;
    uint64_t best=0;
    if (backup_.read) for(unsigned i=0;i<2;++i) {
      const size_t bytes=backup_.read(i,verification_,kMaxBytes,backup_.context);
      GhostCheckpoint candidate;
      if(decode(verification_,bytes,candidate) && candidate.generation>best) {
        best=candidate.generation;backup_slot=1-i;
      }
    }
    backup_save_ok_=writeStore(backup_,backup_slot,buffer_,size);
  }
  return primary_save_ok_;
}
} }
