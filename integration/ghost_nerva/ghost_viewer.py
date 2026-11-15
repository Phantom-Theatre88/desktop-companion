"""Serve the standalone Ghost Viewer on local Mac only; no Ghost ownership."""
from __future__ import annotations

import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parent


class ViewerHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(ROOT), **kwargs)

    def do_GET(self):
        if self.path not in ("/", "/ghost_viewer.html"):
            self.send_error(404, "Not found")
            return
        self.path = "/ghost_viewer.html"
        super().do_GET()

    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Content-Security-Policy",
                         "default-src 'none'; script-src 'unsafe-inline'; "
                         "style-src 'unsafe-inline'; connect-src ws://127.0.0.1:8766; "
                         "img-src 'none'; base-uri 'none'; form-action 'none'")
        super().end_headers()


def main() -> None:
    parser = argparse.ArgumentParser(description="AKARI Ghost Viewer v0.1")
    parser.add_argument("--port", type=int, default=8767)
    args = parser.parse_args()
    if args.port != 8767:
        parser.error("v0.1 uses fixed port 8767 for local Origin validation")
    with ThreadingHTTPServer(("127.0.0.1", args.port), ViewerHandler) as server:
        print(f"Open http://127.0.0.1:{args.port}/ in the Mac browser", flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
