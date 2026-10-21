#pragma once
// ESP32 platform boundary only. The codec and lifecycle do not depend on it.
#include <Preferences.h>
#include <SD.h>
#include "GhostPersistence.h"
namespace deskbot { namespace ghost {
inline const char* checkpointKey(unsigned slot) { return slot ? "slot1" : "slot0"; }
inline size_t readNvsCheckpoint(unsigned slot,uint8_t* data,size_t capacity,void*) {
  Preferences prefs;
  if(!prefs.begin("desk-ghost",true)) return 0;
  const size_t size=prefs.getBytesLength(checkpointKey(slot));
  const size_t read=size && size<=capacity ? prefs.getBytes(checkpointKey(slot),data,size) : 0;
  prefs.end();return read==size?read:0;
}
inline bool writeNvsCheckpoint(unsigned slot,const uint8_t* data,size_t size,void*) {
  Preferences prefs;
  if(!prefs.begin("desk-ghost",false)) return false;
  const bool ok=prefs.putBytes(checkpointKey(slot),data,size)==size;
  prefs.end();return ok;
}
inline const char* sdCheckpointPath(unsigned slot) {
  return slot ? "/ghost-checkpoint1.bin" : "/ghost-checkpoint0.bin";
}
inline size_t readSdCheckpoint(unsigned slot,uint8_t* data,size_t capacity,void*) {
  // Mounting/pin ownership belongs to the board setup. No card is nonfatal.
  if(SD.cardType()==CARD_NONE) return 0;
  auto file=SD.open(sdCheckpointPath(slot),FILE_READ);
  if(!file) return 0;
  const size_t size=file.size();
  const size_t read=size && size<=capacity ? file.read(data,size) : 0;
  file.close();return read==size?read:0;
}
inline bool writeSdCheckpoint(unsigned slot,const uint8_t* data,size_t size,void*) {
  if(SD.cardType()==CARD_NONE) return false;
  const char* path=sdCheckpointPath(slot);
  // Only the inactive backup slot is replaced. The valid other slot survives
  // short writes/power loss. FILE_WRITE must not append to an old checkpoint.
  if(SD.exists(path) && !SD.remove(path)) return false;
  auto file=SD.open(path,FILE_WRITE);
  if(!file) return false;
  const bool ok=file.write(data,size)==size;
  file.flush();file.close();return ok;
}
inline CheckpointStore nvsCheckpointStore() { return {readNvsCheckpoint,writeNvsCheckpoint,nullptr}; }
inline CheckpointStore sdCheckpointStore() { return {readSdCheckpoint,writeSdCheckpoint,nullptr}; }
} }
