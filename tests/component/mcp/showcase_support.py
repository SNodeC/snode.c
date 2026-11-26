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

"""Process support for the separate MCP showcase's complete feature tour."""

import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time


class Server:
    def __init__(self, executable, directory, tls=False, extra=(), header_roots=None):
        self.tls = tls
        self.directory = Path(directory)
        self.log_path = self.directory / "server.log"
        self.log = self.log_path.open("wb")
        self.cert = self.directory / "cert.pem"
        key = self.directory / "key.pem"
        roots = header_roots or (str(Path(__file__).resolve().parents[3] / "src"),) * 2
        args = [executable, "--config-file", "/dev/null", "--log-console", "stderr"]
        args += ["mcp-http", "local", "--host", "127.0.0.1", "--port", "0", "mcp", "--page-size", "3", "--deadline-ms", "2000",
                 "--allowed-origin", "http://localhost:3000", "--header-root", str(roots[0])] + list(extra)
        if tls:
            subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-subj", "/CN=localhost",
                            "-addext", "subjectAltName=IP:127.0.0.1,DNS:localhost", "-keyout", str(key), "-out", str(self.cert), "-days", "1"],
                           check=True, capture_output=True)
            args += ["mcp-https", "local", "--host", "127.0.0.1", "--port", "0", "tls", "--cert", str(self.cert), "--cert-key", str(key),
                     "mcp", "--page-size", "3", "--deadline-ms", "2000", "--allowed-origin", "http://localhost:3000", "--header-root", str(roots[1])] + list(extra)
        self.process = subprocess.Popen(args, stdout=self.log, stderr=self.log,
                                        env=dict(os.environ, XDG_CONFIG_HOME=str(self.directory), XDG_STATE_HOME=str(self.directory)))
        ports = {}
        expected = {"HTTP", "HTTPS"} if tls else {"HTTP"}
        until = time.monotonic() + 8
        while time.monotonic() < until and self.process.poll() is None and not expected <= ports.keys():
            text = self.log_path.read_text(errors="replace")
            for kind, port in re.findall(r"showcase (HTTPS?) listening on 127\.0\.0\.1:(\d+)", text):
                ports[kind] = int(port)
            time.sleep(0.02)
        assert expected <= ports.keys(), self.log_path.read_text(errors="replace")
        self.ports = ports
        self.port = ports["HTTPS" if tls else "HTTP"]
        self.url = ("https" if tls else "http") + "://127.0.0.1:" + str(self.port) + "/mcp"

    def stop(self):
        if self.process.poll() is None:
            self.process.send_signal(signal.SIGTERM)
        self.process.wait(timeout=5)
        self.log.close()
        output = self.log_path.read_text(errors="replace")
        assert self.process.returncode == 241, (self.process.returncode, output)
        assert "AddressSanitizer" not in output and "runtime error:" not in output, output


def records(output):
    return [json.loads(line) for line in output.splitlines() if line.startswith("{")]


class ReplayProxy:
    """Opaque TCP relay: cut one seeded legacy GET and observe the client's reconnect."""

    def __init__(self, port):
        import socketserver
        import threading
        import select
        import socket
        self.cut = False
        self.requests = []
        self.connections = []
        self.lock = threading.Lock()
        self.stopping = threading.Event()
        owner = self

        class Relay(socketserver.BaseRequestHandler):
            def handle(self):
                upstream = socket.create_connection(("127.0.0.1", port), timeout=5)
                with owner.lock:
                    owner.connections.extend([self.request, upstream])
                request_head = bytearray()
                response = bytearray()
                method = None
                seeded = False
                try:
                    live = True
                    while live and not owner.stopping.is_set():
                        readable, _, _ = select.select([self.request, upstream], [], [], 0.2)
                        for source in readable:
                            data = source.recv(65536)
                            if not data:
                                live = False
                            else:
                                destination = upstream if source is self.request else self.request
                                destination.sendall(data)
                                if source is self.request and method is None:
                                    request_head.extend(data)
                                    if b"\r\n\r\n" in request_head:
                                        lines = bytes(request_head).split(b"\r\n\r\n", 1)[0].decode().split("\r\n")
                                        method = lines[0].split()[0]
                                        headers = dict(line.split(":", 1) for line in lines[1:])
                                        headers = {key.lower(): value.strip() for key, value in headers.items()}
                                        seeded = method == "GET" and "last-event-id" not in headers
                                        with owner.lock:
                                            owner.requests.append({"method": method, "lastEventId": headers.get("last-event-id"),
                                                                   "session": headers.get("mcp-session-id")})
                                elif source is upstream and seeded:
                                    response.extend(data)
                                    if b"id:" in response and b"data:" in response and b"\n\n" in response:
                                        with owner.lock:
                                            if not owner.cut:
                                                owner.cut = True
                                                live = False
                except (OSError, ValueError):
                    if not owner.stopping.is_set():
                        raise
                finally:
                    upstream.close()
                    self.request.close()

        class RelayServer(socketserver.ThreadingTCPServer):
            daemon_threads = True

        self.server = RelayServer(("127.0.0.1", 0), Relay)
        self.port = self.server.server_address[1]
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()

    def stop(self):
        self.stopping.set()
        self.server.shutdown()
        for connection in self.connections:
            connection.close()
        self.server.server_close()
        self.thread.join(timeout=3)
