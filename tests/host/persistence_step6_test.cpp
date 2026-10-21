#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include "../../firmware/cores3_base/src/ghost/GhostCore.h"
#include "../../firmware/cores3_base/src/ghost/GhostEsp32Storage.h"
using namespace deskbot;
using namespace deskbot::ghost;
static uint64_t clock_value=1800000000ULL;
static bool clock_ok=true;
static bool clockSource(uint64_t& out,void*) {out=clock_value;return clock_ok;}
static bool near(float a,float b) {return std::fabs(a-b)<1e-5f;}
static void configure(GhostCore& g,bool clock=true) {
  g.configurePersistence(nvsCheckpointStore(),sdCheckpointStore());
  if(clock) g.timeEngine().setUnixClock(clockSource);
}
static void touch(GhostCore& g,uint32_t at=100) {
  g.tick(at);
  auto n=nerve::makeNeuron(nerve::NeuronType::TOUCH,nerve::NeuronSource::TOUCH,at);
  n.person_id=7;g.onNeuron(n);
}
static size_t diskRead(unsigned slot,uint8_t* out,size_t cap,void* ctx) {
  std::string path=static_cast<const char*>(ctx);path+=std::to_string(slot);
  std::ifstream f(path,std::ios::binary|std::ios::ate);
  if(!f)return 0;
  const auto end=f.tellg();if(end<=0 || static_cast<size_t>(end)>cap)return 0;
  const size_t n=static_cast<size_t>(end);f.seekg(0);f.read(reinterpret_cast<char*>(out),n);
  return f? n:0;
}
static bool diskWrite(unsigned slot,const uint8_t* data,size_t size,void* ctx) {
  std::string path=static_cast<const char*>(ctx);path+=std::to_string(slot);
  std::ofstream f(path,std::ios::binary|std::ios::trunc);
  f.write(reinterpret_cast<const char*>(data),size);f.close();return !f.fail();
}
int main(int argc,char** argv) {
  if(argc==3) {
    GhostCore g;g.configurePersistence({diskRead,diskWrite,argv[2]});
    g.timeEngine().setUnixClock(clockSource);g.begin(0);
    if(std::string(argv[1])=="write") {touch(g);assert(g.saveCheckpoint());}
    else {
      assert(g.persistence().source()==CheckpointSource::PRIMARY);
      assert(g.memoryEngine().workingExperienceCount()==1);
      assert(g.relationshipEngine().evaluate(7).positive_experience_count==1);
      auto call=nerve::makeNeuron(nerve::NeuronType::WAKE_WORD_DETECTED,nerve::NeuronSource::PI5,100);
      call.person_id=7;g.tick(100);g.onNeuron(call);
      assert(g.behaviorEngine().interpersonalDecision().response==InterpersonalResponse::OPEN);
      assert(g.memoryEngine().lastCandidate().session_id==2);
    }
    std::cout<<"PASS: separate-process file checkpoint "<<argv[1]<<"\n";return 0;
  }
  Preferences::bytes.clear(); SD.card=0;
  GhostCore first;configure(first);first.begin(0);touch(first);
  const auto saved=first.heart();assert(first.saveCheckpoint());
  assert(first.persistence().primarySaveOk() && !first.persistence().backupSaveOk());
  GhostCore immediate;configure(immediate);immediate.begin(0);
  assert(immediate.offlineTimeStatus()==ElapsedStatus::KNOWN);
  assert(near(immediate.heart().mood,saved.mood));
  assert(near(immediate.heart().attention,saved.attention));
  assert(immediate.memory().event_count==0); // old uptime hints are not reused
  MemoryRecord record;assert(immediate.memoryEngine().recallLatestForPerson(nerve::NeuronType::TOUCH,7,record));
  assert(record.session_id==1 && record.occurred_ms==100);
  assert(immediate.relationshipEngine().evaluate(7).experience_count==1);
  assert(!immediate.behaviorEngine().interpersonalResponseActive());
  clock_value+=10;
  GhostCore shortOff;configure(shortOff);shortOff.begin(0);
  assert(near(shortOff.heart().mood,saved.mood));
  clock_value+=20;
  GhostCore mediumOff;configure(mediumOff);mediumOff.begin(0);
  assert(mediumOff.heart().mood<saved.mood && mediumOff.heart().mood>.60f);
  assert(near(mediumOff.heart().affection,saved.affection));
  clock_value=1800000000ULL+86400;
  GhostCore later;configure(later);later.begin(0);
  assert(near(later.heart().mood,later.heartEngine().baseline().mood));
  assert(near(later.heart().affection,saved.affection));
  assert(later.relationshipEngine().evaluate(7).experience_count==1);
  assert(later.memoryEngine().recallLatestForPerson(nerve::NeuronType::TOUCH,7,record));
  assert(near(record.outcome_context.state.mood,saved.mood)); // facts do not decay
  clock_ok=false;
  GhostCore unknown;configure(unknown);unknown.begin(0);
  assert(unknown.offlineTimeStatus()==ElapsedStatus::CLOCK_UNKNOWN);
  assert(unknown.heartEngine().restorePending());
  assert(near(unknown.heart().mood,saved.mood));
  assert(unknown.relationshipEngine().evaluate(7).experience_count==1);
  clock_ok=true;clock_value=1799999999ULL;
  GhostCore reversed;configure(reversed);reversed.begin(0);
  assert(reversed.offlineTimeStatus()==ElapsedStatus::CLOCK_REVERSED);
  assert(near(reversed.heart().attention,saved.attention));
  clock_value=1800000000ULL;

  // Actual platform adapters through host NVS/SD shims: independent backup.
  SD.card=1;touch(first,200);assert(first.saveCheckpoint());
  assert(first.persistence().backupSaveOk());
  assert(first.persistence().generation()==2);
  auto corrupt=Preferences::bytes["desk-ghost:slot0"];
  Preferences::bytes["desk-ghost:slot0"][30]^=1;
  GhostCore previous;configure(previous);previous.begin(0);
  assert(previous.persistence().generation()==1);
  assert(previous.memoryEngine().workingExperienceCount()==1);
  Preferences::bytes["desk-ghost:slot1"]=std::vector<uint8_t>{1,2,3};
  GhostCore backup;configure(backup);backup.begin(0);
  assert(backup.persistence().source()==CheckpointSource::BACKUP);
  assert(backup.memoryEngine().workingExperienceCount()==2);
  assert(backup.relationshipEngine().evaluate(7).experience_count==2);
  assert(backup.saveCheckpoint()); // primary repair from coherent backup

  // Short write: latest previous slot survives. Failed primary does not touch backup.
  const auto backup_files=SD.files;
  Preferences::write_limit=12;
  touch(backup,300);assert(!backup.saveCheckpoint());
  assert(SD.files==backup_files);
  Preferences::write_limit=SIZE_MAX;
  GhostCore afterFail;configure(afterFail);afterFail.begin(0);
  assert(afterFail.memoryEngine().workingExperienceCount()==2);
  // Missed backup generations must alternate from backup's own valid slot.
  SD.card=0;assert(afterFail.saveCheckpoint());assert(afterFail.saveCheckpoint());
  SD.card=1;SD.write_limit=8;assert(afterFail.saveCheckpoint());
  assert(!afterFail.persistence().backupSaveOk());
  Preferences::bytes.erase("desk-ghost:slot0");Preferences::bytes.erase("desk-ghost:slot1");
  SD.write_limit=SIZE_MAX;
  GhostCore afterBackupFail;configure(afterBackupFail);afterBackupFail.begin(0);
  assert(afterBackupFail.persistence().source()==CheckpointSource::BACKUP);

  // Canonical codec: truncation, every byte corrupted, unknown version, bad values.
  GhostCheckpoint snapshot;
  assert(GhostPersistence::decode(corrupt.data(),corrupt.size(),snapshot));
  std::vector<uint8_t> encoded(GhostPersistence::kMaxBytes);
  const size_t size=GhostPersistence::encode(snapshot,encoded.data(),encoded.size());
  assert(size==corrupt.size());
  for(size_t i=0;i<size;++i) {
    encoded[i]^=0x80;GhostCheckpoint output;
    assert(!GhostPersistence::decode(encoded.data(),size,output));encoded[i]^=0x80;
  }
  for(size_t n=0;n<size;++n) {
    GhostCheckpoint output;assert(!GhostPersistence::decode(encoded.data(),n,output));
  }
  snapshot.heart.mood=NAN;assert(!GhostPersistence::encode(snapshot,encoded.data(),encoded.size()));
  snapshot.heart.mood=.6f;snapshot.memory.count=9;
  assert(!GhostPersistence::encode(snapshot,encoded.data(),encoded.size()));
  snapshot.memory.count=1;snapshot.memory.records[0].has_outcome=false;
  assert(!GhostPersistence::encode(snapshot,encoded.data(),encoded.size()));

  // Unknown schema with a valid CRC is still rejected. Decode never mutates
  // caller output on failure.
  auto checksum=[](const uint8_t* bytes,size_t count) {
    uint32_t crc=0xffffffffu;
    for(size_t i=0;i<count;++i) {
      crc^=bytes[i];
      for(int bit=0;bit<8;++bit) crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));
    }
    return ~crc;
  };
  assert(GhostPersistence::decode(corrupt.data(),corrupt.size(),snapshot));
  const auto originalGeneration=snapshot.generation;
  auto wrongVersion=corrupt;wrongVersion[4]=2;
  const uint32_t crc=checksum(wrongVersion.data(),wrongVersion.size()-4);
  for(unsigned i=0;i<4;++i) wrongVersion[wrongVersion.size()-4+i]=static_cast<uint8_t>(crc>>(8*i));
  assert(!GhostPersistence::decode(wrongVersion.data(),wrongVersion.size(),snapshot));
  assert(snapshot.generation==originalGeneration);

  // First boot and legacy Heart-only fallback preserve valid temporary state.
  Preferences::bytes.clear();SD.card=0;
  HeartPersistence legacy;
  HeartState old;old.mood=.82f;old.attention=.81f;
  assert(legacy.savePrimary(old));
  GhostCore migrated;configure(migrated,false);migrated.begin(0);
  assert(migrated.persistence().source()==CheckpointSource::NONE);
  assert(near(migrated.heart().mood,old.mood));
  assert(near(migrated.heart().attention,old.attention));
  assert(migrated.heartEngine().restorePending());
  old.mood=NAN;assert(legacy.savePrimary(old));
  GhostCore badLegacy;configure(badLegacy,false);badLegacy.begin(0);
  assert(std::isfinite(badLegacy.heart().mood));

  // Pending event excluded; volatile flags never saved; session IDs distinguish boots.
  auto n=nerve::makeNeuron(nerve::NeuronType::SHAKE,nerve::NeuronSource::IMU,1);
  immediate.memoryEngine().beginNeuronExperience(n,immediate.heartContext(1));
  assert(immediate.memoryEngine().exportSnapshot().count==1);
  touch(immediate,400);assert(immediate.memoryEngine().lastCandidate().session_id==2);
  // Rate-limited retries even when storage fails, and monotonic wrap.
  Preferences::bytes.clear();SD.card=0;
  GhostCore periodic;configure(periodic,false);periodic.begin(UINT32_MAX-99);
  touch(periodic,100);periodic.tick(59899);assert(periodic.persistence().generation()==0);
  periodic.tick(59900);assert(periodic.persistence().generation()==1);
  assert(periodic.timeEngine().uptimeMs()==60000);
  periodic.tick(59901);assert(periodic.persistence().generation()==1);
  Preferences::fail_open=true;touch(periodic,60000);periodic.tick(119900);
  assert(!periodic.persistence().primarySaveOk());
  Preferences::fail_open=false;periodic.tick(119901);assert(periodic.persistence().generation()==1);
  periodic.tick(179900);assert(periodic.persistence().generation()==2);
  std::cout<<"PASS: Step 6 coherent NVS/SD checkpoints, CRC/validation, generations, short writes, backup recovery, known/unknown/reversed clock, immutable experience, offline recovery, sessions, periodic retry/wrap\n";
}
