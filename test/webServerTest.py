#!/usr/bin/env python3

"""
This spawns a webserver with API callbacks meant to emulate a device
Can be used to test the webpage separately from the ESP8266
This file should be ran from this local directory
"""

from http.server import HTTPServer, SimpleHTTPRequestHandler
from urllib.parse import urlparse
import json
import os
import threading
import time
import argparse

HOST = "localhost"
PORT = 8080
BASE_PATH = os.path.normpath(
    os.path.join(os.path.dirname(__file__), "..", "data/web")
)

class DeviceState:
    def __init__(self):
        self._data = {}
        self._lock = threading.Lock()

    def get(self, key, default=None):
        with self._lock:
            return self._data.get(key, default)

    def set(self, key, value):
        with self._lock:
            self._data[key] = value

    def update(self, mapping: dict):
        with self._lock:
            self._data.update(mapping)


class Router:
    def __init__(self):
        self._routes = {}

    def route(self, method: str, path: str):
        def decorator(func):
            self._routes[(method.upper(), path)] = func
            return func
        return decorator

    def dispatch(self, handler, method: str, path: str) -> bool:
        fn = self._routes.get((method.upper(), path))
        if not fn:
            return False
        fn(handler)
        return True


class APIHandler(SimpleHTTPRequestHandler):
    state: DeviceState = None
    router: Router = None
    base_path: str = BASE_PATH

    def __init__(self, *args, **kwargs):
        self.directory = self.base_path
        super().__init__(*args, **kwargs)

    def translate_path(self, path):
        orig = super().translate_path(path)
        rel = os.path.relpath(orig, os.getcwd())
        return os.path.join(self.base_path, rel)

    def json_response(self, payload=None, status=200):
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        if payload is not None:
            self.wfile.write(json.dumps(payload).encode())

    def read_json(self):
        length = int(self.headers.get("Content-Length", 0))
        if length == 0:
            return {}
        try:
            return json.loads(self.rfile.read(length))
        except json.JSONDecodeError:
            return None

    def do_GET(self):
        time.sleep(self.state.get("d.responseDelay", 0))
        path = urlparse(self.path).path
        if self.router.dispatch(self, "GET", path):
            return
        super().do_GET()

    def do_POST(self):
        time.sleep(self.state.get("d.responseDelay", 0))
        path = urlparse(self.path).path
        if self.router.dispatch(self, "POST", path):
            return
        self.json_response({"error": "unknown route"}, 404)

    def do_DELETE(self):
        time.sleep(self.state.get("d.responseDelay", 0))
        path = urlparse(self.path).path
        if self.router.dispatch(self, "DELETE", path):
            return
        self.json_response({"error": "unknown route"}, 404)


router = Router()

@router.route("GET", "/api/v1/wifi/status")
def wifi_status(h: APIHandler):
    time.sleep(h.state.get("d.getActionDelay", 0))
    h.json_response({
        "connected": h.state.get("wifi.connected"),
        "ssid": h.state.get("wifi.ssid"),
        "ip": h.state.get("wifi.ip"),
    })


@router.route("GET", "/api/v1/wifi/scan")
def wifi_scan(h: APIHandler):
    time.sleep(h.state.get("d.getActionDelay", 0))
    h.json_response(h.state.get("wifi.networks"))


@router.route("POST", "/api/v1/wifi/connect")
def wifi_connect(h: APIHandler):
    data = h.read_json()
    if data is None:
        return h.json_response({"error": "invalid json"}, 400)

    ssid = data.get("ssid", "")
    ip = "4.5.6.7"

    h.state.update({
        "wifi.connected": True,
        "wifi.ssid": ssid,
        "wifi.ip": ip,
    })

    time.sleep(h.state.get("d.wifiConnDelay", 0))
    h.json_response({"status": "connected", "ssid": ssid, "ip": ip})


@router.route("GET", "/api/v1/ntp/status")
def ntp_status(h: APIHandler):
    time.sleep(h.state.get("d.getActionDelay", 0))
    h.json_response({
        "lastOk": h.state.get("ntp.lastOk"),
        "lastStatus": h.state.get("ntp.lastStatus"),
        "lastSyncTime": h.state.get("ntp.lastSyncTime"),
    })


@router.route("GET", "/api/v1/ntp/config")
def ntp_config_get(h: APIHandler):
    time.sleep(h.state.get("d.getActionDelay", 0))
    h.json_response({"ntp_server": h.state.get("ntp.server")})


@router.route("POST", "/api/v1/ntp/config")
def ntp_config_set(h: APIHandler):
    data = h.read_json()
    if data is None:
        return h.json_response({"error": "invalid json"}, 400)

    server = data.get("ntp_server", "")
    h.state.set("ntp.server", server)
    h.json_response({"status": "ok", "ntp_server": server})


@router.route("POST", "/api/v1/ntp/sync")
def ntp_sync(h: APIHandler):
    h.state.update({
        "ntp.lastStatus": "syncing",
        "ntp.lastSyncTime": time.time(),
        "ntp.lastOk": True,
    })
    h.json_response({"status": "ok"})


@router.route("GET", "/api/v1/ota/status")
def ota_status(h: APIHandler):
    time.sleep(h.state.get("d.getActionDelay", 0))
    h.json_response({
        "inProgress": h.state.get("ota.inProgress"),
        "bytesWritten": h.state.get("ota.bytesWritten"),
        "totalBytes": h.state.get("ota.totalBytes"),
        "error": h.state.get("ota.error"),
        "message": h.state.get("ota.message"),
    })


@router.route("POST", "/api/v1/ota/fw")
@router.route("POST", "/api/v1/ota/fs")
def ota_start(h: APIHandler):
    total = int(h.headers.get("Content-Length", 0))
    h.state.update({
        "ota.inProgress": True,
        "ota.bytesWritten": 0,
        "ota.totalBytes": total,
        "ota.error": False,
        "ota.message": "upload started",
    })
    h.json_response({"status": "upload started"})


@router.route("POST", "/api/v1/ota/cancel")
def ota_cancel(h: APIHandler):
    h.state.update({
        "ota.inProgress": False,
        "ota.error": True,
        "ota.message": "cancelled",
    })
    h.json_response({"status": "cancelled"})


@router.route("GET", "/api/v1/gif")
def gif_list(h: APIHandler):
    time.sleep(h.state.get("d.getActionDelay", 0))
    h.json_response(h.state.get("gif.list"))


@router.route("POST", "/api/v1/gif")
def gif_upload(h: APIHandler):
    data = h.read_json()
    name = data.get("name", "uploaded.gif") if data else "uploaded.gif"
    h.json_response({"status": "success", "filename": name})


@router.route("DELETE", "/api/v1/gif")
def gif_delete(h: APIHandler):
    data = h.read_json()
    if not data or "name" not in data:
        return h.json_response({"error": "missing name"}, 400)
    h.json_response({"status": "success", "file": data["name"]})


@router.route("POST", "/api/v1/gif/play")
def gif_play(h: APIHandler):
    data = h.read_json()
    if not data:
        return h.json_response({"error": "invalid json"}, 400)
    h.state.set("gif.playing", data.get("name"))
    h.json_response({"status": "playing"})


@router.route("POST", "/api/v1/gif/stop")
def gif_stop(h: APIHandler):
    h.state.set("gif.playing", None)
    h.json_response({"status": "stopped"})


@router.route("POST", "/api/v1/reboot")
def reboot(h: APIHandler):
    h.json_response({"status": "rebooting"})

def make_handler(state: DeviceState, router: Router):
    class BoundHandler(APIHandler):
        pass
    BoundHandler.state = state
    BoundHandler.router = router
    BoundHandler.base_path = BASE_PATH
    return BoundHandler


def run_server(httpd: HTTPServer):
    httpd.serve_forever(poll_interval=0.2)

def run_cli(httpd: HTTPServer, state: DeviceState):
    while True:
        try:
            cmd = input("").strip().split()
        except (EOFError, KeyboardInterrupt):
            cmd = ["quit"]

        if not cmd:
            continue

        if cmd[0] in ("q", "quit", "exit"):
            httpd.shutdown()
            httpd.server_close()
            print("Server stopped")
            return

        if cmd[0] == "set" and len(cmd) >= 3:
            state.set(cmd[1], cmd[2])
            print("OK")
        else:
            print("Commands: set <key> <value>, quit")

if __name__ == "__main__":
    state = DeviceState()

    state.set("d.wifiConnDelay", 1)
    state.set("d.responseDelay", 0)
    state.set("d.getActionDelay", 0.5)

    state.update({
        "wifi.ip": "1.2.3.4",
        "wifi.connected": True,
        "wifi.ssid": "TestSSID",
        "wifi.networks": [
            {"ssid": "ABC", "rssi": 0, "enc": 7},
            {"ssid": "Hi There!", "rssi": -50, "enc": 5},
        ],
    })

    state.set("gif.list", {
        "usedBytes": 1500,
        "totalBytes": 50000,
        "freeBytes": 48500,
        "files": [
            {"name": "test.gif", "size": 1000},
            {"name": "[BIG SHOT].gif", "size": 500},
        ],
    })

    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--host', '-h', default=HOST, help='Host to bind (default: %(default)s)')
    parser.add_argument('--port', '-P', type=int, default=PORT, help='Port to bind (default: %(default)s)')
    parser.add_argument('--help', action='help', help='show this help message and exit')
    args = parser.parse_args()

    bind_host = args.host
    bind_port = args.port

    HandlerClass = make_handler(state, router)
    httpd = HTTPServer((bind_host, bind_port), HandlerClass)

    threading.Thread(target=run_server, args=(httpd,), daemon=True).start()
    print(f"HTTP listening on http://{bind_host}:{bind_port} (serving {BASE_PATH}/)")

    run_cli(httpd, state)
