import http.server
import socketserver
import webbrowser
import os
import sys
import mimetypes

# Ensure .avif MIME type is registered
mimetypes.add_type('image/avif', '.avif')
mimetypes.add_type('application/json', '.json')

PORT = 5050
DIRECTORY = os.path.dirname(os.path.abspath(__file__))

class SatisToolsHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)
    
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Cache-Control', 'no-cache, no-store, must-revalidate')
        super().end_headers()

def main():
    os.chdir(DIRECTORY)
    # Allow port reuse
    socketserver.TCPServer.allow_reuse_address = True
    try:
        with socketserver.TCPServer(("", PORT), SatisToolsHandler) as httpd:
            url = f"http://localhost:{PORT}/map_preview.html"
            print("=======================================================")
            print("  SatisTools - Интерактивная карта Satisfactory (v1.2)")
            print(f"  Сервер запущен: {url}")
            print("  Для выхода нажмите Ctrl+C")
            print("=======================================================")
            webbrowser.open(url)
            httpd.serve_forever()
    except OSError as e:
        if e.errno == 10048: # Port already in use, try alternate
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
