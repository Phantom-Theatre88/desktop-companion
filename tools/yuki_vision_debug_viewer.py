#!/usr/bin/env python3
"""Yuki vision debug viewer for macOS.

Reads the StackChan USB serial debug stream and serves a local browser view
showing the downsampled camera image plus the face detector bounding box.
No third-party Python packages are required.
"""

from __future__ import annotations

import argparse
import base64
import json
import os
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import termios

STATE_LOCK = threading.Lock()
STATE = {
    "seq": -1,
    "frame_width": 0,
    "frame_height": 0,
    "debug_width": 0,
    "debug_height": 0,
    "face": False,
    "x1": -1,
    "y1": -1,
    "x2": -1,
    "y2": -1,
    "cx": -1,
    "cy": -1,
    "pixels_b64": "",
    "updated": 0.0,
}

HTML = r'''<!doctype html>
<meta charset="utf-8">
<title>Yuki Vision Debug</title>
<style>
  :root { color-scheme: dark; font-family: -apple-system, BlinkMacSystemFont, sans-serif; }
  body { margin: 0; background: #111; color: #eee; }
  main { max-width: 980px; margin: 24px auto; padding: 0 18px; }
  .row { display: flex; gap: 18px; flex-wrap: wrap; align-items: flex-start; }
  .panel { background: #1a1a1a; border: 1px solid #333; border-radius: 12px; padding: 14px; }
  canvas { width: 640px; max-width: 100%; height: auto; image-rendering: pixelated; background: #000; }
  .status { font-weight: 700; font-size: 18px; margin-bottom: 8px; }
  .face { color: #56e37d; }
  .no-face { color: #ff6b6b; }
  code { color: #d8eaff; }
  .meta { min-width: 250px; line-height: 1.7; }
  .hint { color: #aaa; font-size: 13px; }
</style>
<main>
  <h1>Yuki Vision Debug</h1>
  <div class="row">
    <div class="panel">
      <canvas id="view" width="640" height="480"></canvas>
      <div class="hint">緑枠 = M5側の顔検出結果 / 赤表示 = 顔未検出</div>
    </div>
    <div class="panel meta">
      <div id="status" class="status no-face">WAITING...</div>
      <div>Frame: <code id="frame">-</code></div>
      <div>BBox: <code id="bbox">-</code></div>
      <div>Center: <code id="center">-</code></div>
      <div>Seq: <code id="seq">-</code></div>
      <div>Age: <code id="age">-</code></div>
    </div>
  </div>
</main>
<script>
const canvas = document.getElementById('view');
const ctx = canvas.getContext('2d');
const imageCanvas = document.createElement('canvas');
const imageCtx = imageCanvas.getContext('2d');

function setText(id, value) { document.getElementById(id).textContent = value; }

async function tick() {
  try {
    const r = await fetch('/state', {cache: 'no-store'});
    const s = await r.json();
    if (s.debug_width > 0 && s.debug_height > 0 && s.pixels_b64) {
      imageCanvas.width = s.debug_width;
      imageCanvas.height = s.debug_height;
      const raw = atob(s.pixels_b64);
      const img = imageCtx.createImageData(s.debug_width, s.debug_height);
      for (let i = 0; i < raw.length; i++) {
        const v = raw.charCodeAt(i);
        const p = i * 4;
        img.data[p] = v; img.data[p+1] = v; img.data[p+2] = v; img.data[p+3] = 255;
      }
      imageCtx.putImageData(img, 0, 0);
      canvas.width = 640;
      canvas.height = 480;
      ctx.imageSmoothingEnabled = false;
      ctx.drawImage(imageCanvas, 0, 0, canvas.width, canvas.height);

      if (s.face && s.frame_width > 0 && s.frame_height > 0) {
        const sx = canvas.width / s.frame_width;
        const sy = canvas.height / s.frame_height;
        ctx.strokeStyle = '#56e37d';
        ctx.lineWidth = 4;
        ctx.strokeRect(s.x1 * sx, s.y1 * sy, (s.x2 - s.x1) * sx, (s.y2 - s.y1) * sy);
        ctx.fillStyle = '#56e37d';
        ctx.beginPath(); ctx.arc(s.cx * sx, s.cy * sy, 6, 0, Math.PI * 2); ctx.fill();
      }
    }
    const status = document.getElementById('status');
    status.textContent = s.face ? 'FACE DETECTED' : 'NO FACE';
    status.className = 'status ' + (s.face ? 'face' : 'no-face');
    setText('frame', `${s.frame_width}×${s.frame_height} → ${s.debug_width}×${s.debug_height}`);
    setText('bbox', s.face ? `(${s.x1},${s.y1}) - (${s.x2},${s.y2})` : '-');
    setText('center', s.face ? `(${s.cx},${s.cy})` : '-');
    setText('seq', s.seq);
    setText('age', s.updated ? `${((Date.now()/1000)-s.updated).toFixed(1)} s` : '-');
  } catch (e) {
    document.getElementById('status').textContent = 'SERIAL / SERVER WAITING';
  }
  setTimeout(tick, 300);
}
tick();
</script>'''


def configure_serial(fd: int, baud: int) -> None:
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[3] = 0
    attrs[4] = getattr(termios, f"B{baud}")
    attrs[5] = getattr(termios, f"B{baud}")
    attrs[6][termios.VMIN] = 1
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)


def serial_reader(port: str, baud: int) -> None:
    while True:
        try:
            fd = os.open(port, os.O_RDONLY | os.O_NOCTTY)
            configure_serial(fd, baud)
            print(f"[serial] connected: {port} @ {baud}")
            with os.fdopen(fd, "rb", buffering=0) as stream:
                pending = None
                chunks: list[str] = []
                buf = b""
                while True:
                    data = stream.read(1024)
                    if not data:
                        raise OSError("serial disconnected")
                    buf += data
                    while b"\n" in buf:
                        raw_line, buf = buf.split(b"\n", 1)
                        line = raw_line.decode("utf-8", errors="ignore").strip()
                        if line.startswith("YUKI_DBG_BEGIN "):
                            parts = line.split()
                            if len(parts) == 13:
                                pending = {
                                    "seq": int(parts[1]),
                                    "frame_width": int(parts[2]),
                                    "frame_height": int(parts[3]),
                                    "debug_width": int(parts[4]),
                                    "debug_height": int(parts[5]),
                                    "face": parts[6] == "1",
                                    "x1": int(parts[7]), "y1": int(parts[8]),
                                    "x2": int(parts[9]), "y2": int(parts[10]),
                                    "cx": int(parts[11]), "cy": int(parts[12]),
                                }
                                chunks = []
                        elif line.startswith("YUKI_DBG_DATA ") and pending is not None:
                            chunks.append(line[len("YUKI_DBG_DATA "):])
                        elif line.startswith("YUKI_DBG_END ") and pending is not None:
                            encoded = "".join(chunks)
                            try:
                                raw = base64.b64decode(encoded, validate=True)
                            except Exception:
                                pending = None
                                chunks = []
                                continue
                            expected = pending["debug_width"] * pending["debug_height"]
                            if len(raw) == expected:
                                pending["pixels_b64"] = encoded
                                pending["updated"] = time.time()
                                with STATE_LOCK:
                                    STATE.update(pending)
                            pending = None
                            chunks = []
        except Exception as exc:
            print(f"[serial] {exc}; retrying...")
            time.sleep(1.0)


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/state":
            with STATE_LOCK:
                payload = json.dumps(STATE).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return
        payload = HTML.encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def log_message(self, fmt, *args):
        pass


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="/dev/cu.usbmodem1101")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--http-port", type=int, default=8765)
    parser.add_argument("--no-browser", action="store_true")
    args = parser.parse_args()

    thread = threading.Thread(target=serial_reader, args=(args.port, args.baud), daemon=True)
    thread.start()

    server = ThreadingHTTPServer(("127.0.0.1", args.http_port), Handler)
    url = f"http://127.0.0.1:{args.http_port}/"
    print(f"[viewer] {url}")
    print("[viewer] idf.py monitor must be closed because this viewer owns the USB serial port.")
    if not args.no_browser:
        threading.Timer(0.5, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[viewer] stopped")


if __name__ == "__main__":
    main()
