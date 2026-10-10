import http.server
import socketserver
import webbrowser
import os
import sys
import mimetypes
import urllib.request
import urllib.error
import json

# Ensure MIME types
mimetypes.add_type('image/avif', '.avif')
mimetypes.add_type('application/json', '.json')

PORT = 5050
DIRECTORY = os.path.dirname(os.path.abspath(__file__))
FRM_URL = "http://localhost:8080"
AUTH_TOKEN = "H2AZH4bcEQ6jvSu1DbFFfrQXj8UWCLQt"

class SatisToolsHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)
    
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Cache-Control', 'no-cache, no-store, must-revalidate')
        super().end_headers()

    def do_GET(self):
        # Proxy endpoint for FRM
        if self.path.startswith('/frm/'):
            subpath = self.path[5:] # e.g. "getPlayer"
            target = f"{FRM_URL}/{subpath}"
            try:
                req = urllib.request.Request(
                    target,
                    headers={
                        'User-Agent': 'SatisTools',
                        'X-FRM-Authorization': AUTH_TOKEN
                    }
                )
                with urllib.request.urlopen(req, timeout=3) as resp:
                    data = resp.read()
                    self.send_response(resp.status)
                    self.send_header('Content-Type', 'application/json')
                    self.end_headers()
                    self.wfile.write(data)
            except Exception as e:
                self.send_response(503)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                err_payload = json.dumps({"error": str(e), "connected": False})
                self.wfile.write(err_payload.encode('utf-8'))
            return
        
        # Check status endpoint
        if self.path == '/status':
            try:
                req = urllib.request.Request(
                    f"{FRM_URL}/getPlayer",
                    headers={'User-Agent': 'SatisTools', 'X-FRM-Authorization': AUTH_TOKEN}
                )
                with urllib.request.urlopen(req, timeout=2) as resp:
                    status = {"online": True, "code": resp.status}
            except Exception as e:
                status = {"online": False, "reason": str(e)}
            
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(status).encode('utf-8'))
            return

        super().do_GET()

def main():
    os.chdir(DIRECTORY)
    socketserver.TCPServer.allow_reuse_address = True
    try:
        with socketserver.TCPServer(("", PORT), SatisToolsHandler) as httpd:
            url = f"http://localhost:{PORT}/map_preview.html"
            print("=======================================================")
            print("  SatisTools - Интерактивная карта Satisfactory (v1.2)")
            print(f"  Сервер запущен: {url}")
            print(f"  FRM Мост активен на: {FRM_URL}")
            print("=======================================================")
            webbrowser.open(url)
            httpd.serve_forever()
    except OSError as e:
        if e.errno == 10048:
            alt_port = 5055
            with socketserver.TCPServer(("", alt_port), SatisToolsHandler) as httpd:
                url = f"http://localhost:{alt_port}/map_preview.html"
                print(f"SatisTools Server running on alternate port: {url}")
                webbrowser.open(url)
                httpd.serve_forever()
        else:
            raise

if __name__ == '__main__':
    main()
