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

"""Legacy initialized must complete before application exchanges are released."""
import http.client
import http.server
import json
import os
import subprocess
import sys
import tempfile
import threading
import http_test

class Gate(http.server.ThreadingHTTPServer):
    daemon_threads = True

class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    def log_message(self, *_):
        pass
    def do_GET(self):
        self.send_response(405)
        self.send_header('Content-Length', '0')
        self.end_headers()
    def do_POST(self):
        body = self.rfile.read(int(self.headers.get('Content-Length', '0')))
        method = json.loads(body).get('method', '')
        self.server.methods.append(method)
        if self.headers.get('Host') != '127.0.0.1:' + str(self.server.server_address[1]):
            self.server.bad_hosts.append(self.headers.get('Host'))
        if self.server.reject and (self.server.reject != 404 or method == 'server/discover'):
            self.send_response(self.server.reject)
            self.send_header('Content-Length', '0')
            self.end_headers()
        else:
            if method == 'notifications/initialized':
                self.server.application.wait(timeout=0.3)
            elif method not in ('initialize', 'server/discover'):
                self.server.application.set()
                self.server.violations += int(not self.server.initialized)
            upstream = http.client.HTTPConnection('127.0.0.1', self.server.backend, timeout=3)
            upstream.request('POST', self.path, body, dict(self.headers))
            response = upstream.getresponse()
            data = response.read()
            if method == 'notifications/initialized':
                self.server.initialized = True
            self.send_response(response.status)
            for name, value in response.getheaders():
                if name.lower() not in ('connection', 'content-length', 'transfer-encoding'):
                    self.send_header(name, value)
            self.send_header('Content-Length', str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            upstream.close()

client, executable = sys.argv[1:3]
with tempfile.TemporaryDirectory() as directory:
    backend = http_test.Server(executable, directory, False, curl='curl')
    proxy = Gate(('127.0.0.1', 0), Handler)
    proxy.backend = backend.port
    proxy.reject = 0
    proxy.methods = []
    proxy.bad_hosts = []
    proxy.initialized = False
    proxy.violations = 0
    proxy.application = threading.Event()
    worker = threading.Thread(target=proxy.serve_forever)
    worker.start()
    try:
        command = [client, '--config-file', '/dev/null', '--quiet', '--log-console', 'stderr', 'client',
                                 '--legacy', 'true', '--command', 'tools', 'mcp-client-http', 'remote', '--port', str(proxy.server_address[1])]
        environment = dict(os.environ, XDG_CONFIG_HOME=directory, XDG_STATE_HOME=directory, MCP_BEARER_TOKEN='fixture')
        result = subprocess.run(command, capture_output=True, timeout=10, env=environment)
        assert result.returncode == 0, (result.stdout, result.stderr)
        assert proxy.initialized and proxy.application.is_set() and proxy.violations == 0, proxy.violations
        assert json.loads(result.stdout)['tools']
        assert not proxy.bad_hosts, proxy.bad_hosts
        command[command.index('--legacy') + 1] = 'false'
        for status in (401, 500):
            proxy.reject = status
            proxy.methods.clear()
            result = subprocess.run(command, capture_output=True, timeout=10, env=environment)
            assert result.returncode == 1 and proxy.methods == ['server/discover'], (status, proxy.methods, result.stderr)
            assert str(status).encode() in result.stderr, result.stderr
        proxy.reject = 404
        proxy.methods.clear()
        proxy.initialized = False
        proxy.application.clear()
        proxy.violations = 0
        result = subprocess.run(command, capture_output=True, timeout=10, env=environment)
        assert result.returncode == 0 and proxy.methods[:2] == ['server/discover', 'initialize'], (proxy.methods, result.stderr)
        assert proxy.violations == 0 and json.loads(result.stdout)['tools']

    finally:
        proxy.shutdown()
        proxy.server_close()
        worker.join(timeout=2)
        backend.stop()
