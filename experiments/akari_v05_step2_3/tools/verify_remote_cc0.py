#!/usr/bin/env python3
"""Public CC0 source availability and actual GLB animation inspection, no AKARI private assets."""
from __future__ import annotations
import hashlib, io, json, os, pathlib, struct, sys, urllib.request, urllib.error, zipfile
EXPECTED = "64ca5ab4a92c9da079c5e88ca509391b0a58a24ccf2a1307b16e2f0fc6b23ffc"
URLS = [
 "https://huggingface.co/datasets/Kinemiq/free-game-animations/resolve/main/KinemIQ_Free_Animations_CC0_GLB.zip",
 "https://huggingface.co/datasets/Kinemiq/free-game-animations/resolve/main/KinemIQ_Free_Animations_CC0_GLB.zip?download=true",
 "https://hf-mirror.com/datasets/Kinemiq/free-game-animations/resolve/main/KinemIQ_Free_Animations_CC0_GLB.zip",
]
OUT=pathlib.Path(os.environ.get("AKARI_ASSET_REPORT","akari-motion-source-report.json"))
result={"status":"failed","sha256_expected":EXPECTED,"attempts":[],"clips":[]}
def save():
 OUT.parent.mkdir(parents=True,exist_ok=True)
 OUT.write_text(json.dumps(result,ensure_ascii=False,indent=2))
for url in URLS:
 attempt={"url":url}
 result["attempts"].append(attempt)
 try:
  with urllib.request.urlopen(urllib.request.Request(url,headers={"User-Agent":"AKARI-public-asset-verify/1.0"}),timeout=35) as r:
   data=r.read(100_000_001)
  if len(data)>100_000_000:raise ValueError("download too large")
  digest=hashlib.sha256(data).hexdigest()
  attempt["sha256"]=digest
  attempt["bytes"]=len(data)
  if digest!=EXPECTED:raise ValueError("SHA-256 mismatch: reject asset")
  with zipfile.ZipFile(io.BytesIO(data)) as z:
   names=z.namelist()
   attempt["zip_entries"]=len(names)
   for info in z.infolist():
    if not info.filename.lower().endswith(".glb"):continue
    if info.file_size>12_000_000:continue
    payload=z.read(info.filename)
    if payload[:4]!=b"glTF" or len(payload)<20:continue
    length,typ=struct.unpack_from("<II",payload,12)
    if typ!=0x4e4f534a:continue
    doc=json.loads(payload[20:20+length])
    for animation in doc.get("animations",[]):
     channels=animation.get("channels",[])
     bones=sum(1 for c in channels if c.get("target",{}).get("path")=="rotation")
     result["clips"].append({"file":info.filename,"name":animation.get("name",""),"rotation_channels":bones,"total_channels":len(channels)})
  result.update({"status":"download_verified","source":url,"sha256":digest,"bytes":len(data),"glb_clips":len(result["clips"])})
  attempt["status"]="success"
  save()
  print("Verified CC0 bundle SHA-256:",digest,"GLB clips:",len(result["clips"]))
  sys.exit(0)
 except Exception as e:
  attempt["status"]="failure"
  attempt["reason"]=type(e).__name__+": "+str(e)
  print("Source failure:",attempt["reason"],file=sys.stderr)
save()
print("No source was verified; see JSON report",file=sys.stderr)
sys.exit(1)
