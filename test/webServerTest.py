"""
This spawns a webserver with API callbacks meant to emulate a device

Can be used to test the webpage separately from the ESP

Partially ChatGPT generated, with human oversight
"""
from http.server import HTTPServer, SimpleHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
import json
import os
import sys
import threading
import time

HOST = 'localhost'
PORT = 8080
BASE_PATH = '../data/web'

class InfoClass():
    def __init__(self):
        self.info = {}
        self.lock = threading.Lock()
    
    def set(self, key, val):
        with self.lock:
            self.info[key] = val
    
    def get(self, key):
        with self.lock:
            if key not in self.info:
                return ''
            return self.info[key]


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.directory = BASE_PATH

    def translate_path(self, path):
        orig = super().translate_path(path)
        rel = os.path.relpath(orig, os.getcwd())
        return os.path.join(BASE_PATH, rel)
    
    def _json(self, code=200, payload=None):
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        if payload is not None:
            self.wfile.write(json.dumps(payload).encode())
    
    def do_GET(self):
        p = urlparse(self.path).path
        # print(f"Made GET request: {p}")        # todo: debug

        time.sleep(info.get('d.responseDelay'))

        if p == "/api/v1/wifi/status":
            time.sleep(info.get('d.getActionDelay'))
            return self._json(payload={
                'connected': info.get('wifi.connected'),
                'ssid': info.get('wifi.ssid'),
                'ip': info.get('wifi.ip'),
            })
        elif p == "/api/v1/wifi/scan":
            time.sleep(info.get('d.getActionDelay'))
            return self._json(payload=info.get('wifi.networks'))
        elif p == "/api/v1/gif":
            time.sleep(info.get('d.getActionDelay'))
            return self._json(payload=info.get('gif.list'))
        else:
            return super().do_GET()

    def do_POST(self):
        p = urlparse(self.path).path
        # print(f"Made POST request: {p}")        # todo: debug
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length)

        time.sleep(info.get('d.responseDelay'))

        if p == "/api/v1/wifi/connect":
            data = json.loads(body)
            print(data)
            ssid = (data.get("ssid") or '')
            passw = (data.get("password") or '')
            ip = "4.5.6.7"
            print(f"Attempted to connect to SSID '{ssid}' with password '{passw}'")
            info.set("wifi.connected", True)
            info.set("wifi.ssid", ssid)
            info.set("wifi.ip", ip)
            time.sleep(info.get('d.wifiConnDelay'))
            return self._json(payload={"status": 'connected', "ssid": ssid, 'ip': ip})
        elif p == "/api/v1/reboot":
            print("Requested to reboot!")
        else:
            return self._json(404, {"ok": False, "error": "unknown route"})
    
    def do_DELETE(self):
        p = urlparse(self.path).path
        # print(f"Made POST request: {p}")        # todo: debug
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length)

        time.sleep(info.get('d.responseDelay'))
        
        if p == "/api/v1/gif":
            data = json.loads(body)
            print(data)
            name = data['name']
            print(f"Requested to delete GIF with name '{name}'")
            return self._json(payload={"status": 'success', "message": 'file removed', 'file': name})
        else:
            return self._json(404, {"ok": False, "error": "unknown route"})

def run_server(httpd: HTTPServer):
    httpd.serve_forever(poll_interval=0.2)


def run_cli(httpd: HTTPServer):
    while True:
        try:
            line = input("> ").strip()
            line = line.split(' ')
        except (EOFError, KeyboardInterrupt):
            line = "q"
        
        try:
            cmd = line[0]

            if cmd in ['q', 'quit', 'exit']:
                print("Exiting")
                httpd.shutdown()
                httpd.server_close()
                return
            elif cmd == 'set':
                noun = line[1]
                # todo: convert to appropriate type (int, float, str, etc)
                info.set(noun, line[2])
            elif cmd == 'help':
                print("...coming soon!")
            else:
                print("invalid command")
        
        except IndexError:
            print("Invalid command (index error)")



if __name__ == "__main__":
    httpd = HTTPServer((HOST, PORT), Handler)

    # a global variables class that is grabbed by Handler and set by the CLI
    info = InfoClass()

    info.set('d.wifiConnDelay', 1)          # to simulate the wifi connection delay
    info.set('d.responseDelay', 0)        # to simulate a slower network response
    # info.set('d.responseDelay', 0.1)        # to simulate a slower network response
    info.set('d.getActionDelay', 0.5)       # to simulate the esp8266 doing stuff on a task/action

    info.set('wifi.ip', '1.2.3.4')
    info.set('wifi.connected', True)
    info.set('wifi.ssid', 'TestSSID')
    info.set('wifi.networks', [
                {'ssid': 'ABC', 'rssi': 0, 'enc': 7}, 
                {'ssid': 'Hi There!', 'rssi': -50, 'enc': 5}
            ]
    )
    info.set('gif.list', {
        'usedBytes': 1500,
        'totalBytes': 50000,
        'freeBytes': 50000-1500,
        'files': [
            {'name': 'test.gif', 'size': 1000}, 
            {'name': '[BIG SHOT].gif', 'size': 500}
        ],
        }
    )

    t = threading.Thread(target=run_server, args=(httpd,), daemon=True)
    t.start()
    print(f"HTTP listening on http://{HOST}:{PORT} (serving {BASE_PATH}/)")

    run_cli(httpd)