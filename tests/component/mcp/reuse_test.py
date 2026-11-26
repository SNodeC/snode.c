# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# ---------------------------------------------------------------------------
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

"""Every sequential JSON POST page reuses one connection; GET remains separate."""
import http.client
import http.server
import json
import os
import subprocess
import sys
import tempfile
import threading
import http_test

class Server(http.server.ThreadingHTTPServer):
    daemon_threads = True

class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    def setup(self):
        super().setup()
        with self.server.lock:
            self.server.connections += 1
            self.ident = self.server.connections
    def log_message(self, *_):
        pass
    def do_GET(self):
        self.server.gets.add(self.ident)
        self.send_response(405)
        self.send_header('Content-Length','0')
        self.end_headers()
    def do_POST(self):
        self.server.posts.add(self.ident)
        body = self.rfile.read(int(self.headers['Content-Length']))
        method = json.loads(body).get('method')
        self.server.methods.append(method)
        upstream = http.client.HTTPConnection('127.0.0.1', self.server.backend, timeout=3)
        upstream.request('POST',self.path,body,dict(self.headers))
        response = upstream.getresponse()
        data = response.read()
        self.send_response(response.status)
        for key,value in response.getheaders():
            if key.lower() not in ('connection','content-length','transfer-encoding'):
                self.send_header(key,value)
        if self.server.force and self.server.methods.count('tools/list') == 3:
            self.send_header('Connection','close')
            self.close_connection = True
        self.send_header('Content-Length',str(len(data)))
        self.end_headers()
        self.wfile.write(data)
        upstream.close()

with tempfile.TemporaryDirectory() as directory:
    backend = http_test.Server(sys.argv[2],directory,False,extra=('--page-size','1'),curl='curl')
    try:
        for legacy, force in [('false',False),('true',False),('false',True),('true',True)]:
            gate = Server(('127.0.0.1',0),Handler)
            gate.lock = threading.Lock()
            gate.connections = 0
            gate.posts = set()
            gate.gets = set()
            gate.methods = []
            gate.backend = backend.port
            gate.force = force
            worker = threading.Thread(target=gate.serve_forever)
            worker.start()
            try:
                result = subprocess.run([sys.argv[1],'--config-file','/dev/null','--quiet','client','--legacy',legacy,
                                         '--command','tools','mcp-client-http','remote','--port',str(gate.server_address[1])],
                                        capture_output=True,timeout=8,env=dict(os.environ,MCP_BEARER_TOKEN='fixture'))
                assert result.returncode == 0,(result.stdout,result.stderr)
                assert len(json.loads(result.stdout)['tools']) >= 10
                assert gate.methods.count('tools/list') >= 10,gate.methods
                assert len(gate.posts) == (2 if force else 1),(legacy,gate.posts,gate.gets,gate.connections)
                assert gate.posts.isdisjoint(gate.gets),(gate.posts,gate.gets)
                assert gate.connections == (2 if legacy=='true' else 1)+int(force),gate.connections
                print(legacy,gate.connections,'connections',gate.methods.count('tools/list'),'pages PASS')
            finally:
                gate.shutdown()
                gate.server_close()
                worker.join()
    finally:
        backend.stop()
