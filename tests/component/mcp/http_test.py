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

import base64
import json
import os
import signal
import socket
import ssl
import subprocess
import sys
import tempfile
import time

META = {"io.modelcontextprotocol/protocolVersion": "2026-07-28", "io.modelcontextprotocol/clientCapabilities": {}}


def request(method, params=None, ident=1):
    return {"jsonrpc": "2.0", "id": ident, "method": method, "params": dict(params or {}, _meta=META)}


def headers(value):
    method = value.get("method", "")
    fields = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream", "Authorization": "Bearer fixture",
              "MCP-Protocol-Version": "2026-07-28", "Mcp-Method": method}
    if method in ("tools/call", "prompts/get", "resources/read"):
        name = value["params"].get("name", value["params"].get("uri", ""))
        fields["Mcp-Name"] = name
    return fields


class Server:
    def __init__(self, executable, directory, tls, extra=(), curl=None):
        sock = socket.socket()
        sock.bind(("127.0.0.1", 0))
        self.port = sock.getsockname()[1]
        sock.close()
        self.tls = tls
        self.curlCommand = curl or sys.argv[2]
        self.url = ("https" if tls else "http") + "://127.0.0.1:" + str(self.port) + "/mcp"
        self.errors = tempfile.TemporaryFile()
        args = [executable, "--log-console", "stderr", "mcp-http", "local", "--port", str(self.port),
                "mcp", "--allowed-origin", "http://localhost:3000"]
        if tls:
            key, cert = directory + "/key.pem", directory + "/cert.pem"
            subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-subj", "/CN=localhost",
                            "-keyout", key, "-out", cert, "-days", "1"], check=True, capture_output=True)
            args += ["tls", "--cert", cert, "--cert-key", key]
        args += list(extra)
        self.process = subprocess.Popen(args, stdout=self.errors, stderr=self.errors,
                                        env=dict(os.environ, XDG_CONFIG_HOME=directory, XDG_STATE_HOME=directory))
        connected = False
        until = time.monotonic() + 5
        while not connected and time.monotonic() < until and self.process.poll() is None:
            try:
                probe = socket.create_connection(("127.0.0.1", self.port), timeout=0.1)
                probe.close()
                connected = True
            except OSError:
                time.sleep(0.02)
        if not connected:
            self.errors.seek(0)
            raise AssertionError(self.errors.read().decode(errors="replace"))

    def curl(self, value=None, overrides=None, method="POST", raw=None):
        fields = headers(value) if value is not None else {"Authorization": "Bearer fixture"}
        fields.update(overrides or {})
        command = [self.curlCommand, "--silent", "--show-error", "--max-time", "4", "--http1.1", "--insecure", "-i", "-X", method]
        for key, text in fields.items():
            command += ["-H", key + (";" if text == "" else ":" + (" " + text if text is not None else ""))]
        body = json.dumps(value, ensure_ascii=False).encode() if raw is None and value is not None else raw
        if body is not None:
            command += ["--data-binary", "@-"]
        result = subprocess.run(command + [self.url], input=body, capture_output=True, timeout=6)
        assert result.returncode == 0, result.stderr
        head, content = result.stdout.split(b"\r\n\r\n", 1)
        return int(head.split()[1]), head, content

    def connection(self):
        connection = socket.create_connection(("127.0.0.1", self.port), timeout=3)
        if self.tls:
            connection = ssl._create_unverified_context().wrap_socket(connection, server_hostname="localhost")
        return connection

    def stop(self):
        self.process.send_signal(signal.SIGTERM)
        self.process.wait(timeout=4)
        self.errors.seek(0)
        output = self.errors.read()
        assert self.process.returncode == 241, (self.process.returncode, output)
        assert b"AddressSanitizer" not in output, output.decode(errors="replace")
        self.errors.close()


def wire(value, overrides=None):
    body = json.dumps(value).encode()
    fields = headers(value)
    fields.update(overrides or {})
    fields["Content-Length"] = str(len(body))
    fields["Host"] = "localhost"
    return b"POST /mcp HTTP/1.1\r\n" + b"".join((key + ": " + text + "\r\n").encode() for key, text in fields.items()
                                              if text is not None) + b"\r\n" + body


def read_response(connection):
    stream = connection.makefile("rb", buffering=0)
    head = bytearray()
    while not head.endswith(b"\r\n\r\n"):
        data = stream.read(1)
        assert data, head
        head += data
    body = bytearray()
    if b"transfer-encoding: chunked" in head.lower():
        size = int(stream.readline().strip(), 16)
        while size:
            remaining = size
            while remaining:
                data = stream.read(remaining)
                assert data
                body += data
                remaining -= len(data)
            assert stream.read(2) == b"\r\n"
            size = int(stream.readline().strip(), 16)
        assert stream.read(2) == b"\r\n", "exact final zero-chunk termination"
    else:
        length = next(int(line.split(b":", 1)[1]) for line in bytes(head).split(b"\r\n") if line.lower().startswith(b"content-length:"))
        while len(body) < length:
            body += stream.read(length - len(body))
    stream.close()
    return bytes(head), bytes(body)


def legacy_qualification(server):
    fields = {"MCP-Protocol-Version": "2025-11-25", "Mcp-Method": None, "Mcp-Name": None, "Mcp-Session-Id": "ignored"}

    def legacy(method, params=None, ident=40):
        return {"jsonrpc": "2.0", "id": ident, "method": method, "params": params or {}}

    status, head, body = server.curl(legacy("tools/list"), fields)
    assert status == 200 and b"application/json" in head and b"mcp-session-id" not in head.lower()
    assert "resultType" not in json.loads(body)["result"], "legacy HTTP is usable without a session record"
    init = legacy("initialize", {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "test", "version": "1"}})
    status, head, body = server.curl(init, dict(fields, **{"MCP-Protocol-Version": None}))
    assert status == 200 and json.loads(body)["result"]["protocolVersion"] == "2025-11-25"
    assert b"mcp-session-id" in head.lower(), "legacy initialization establishes a bounded notification session"
    bad = legacy("initialize", {"protocolVersion": "2025-11-25"})
    assert json.loads(server.curl(bad, fields)[2])["error"]["code"] == -32602
    assert server.curl(legacy("ping"), fields)[0] == 200
    status, _, body = server.curl(legacy("tools/call", {"name": "overflow"}), fields)
    assert status == 500 and json.loads(body)["error"]["code"] == -32603, "JSON output is bounded too"
    for method in ["GET", "DELETE"]:
        assert server.curl(overrides=fields, method=method)[0] == 404, "unknown legacy session rejected"
    for method in ["server/discover", "subscriptions/listen"]:
        assert server.curl(legacy(method), fields)[0] == 404
    connection = server.connection()
    connection.sendall(wire(legacy("tools/call", {"name": "delay"}, 55), fields))
    cancel = {"jsonrpc": "2.0", "method": "notifications/cancelled", "params": {"requestId": 55}}
    assert server.curl(cancel, fields)[::2] == (202, b"")
    head, body = read_response(connection)
    assert b"application/json" in head and json.loads(body)["id"] == 55, "legacy HTTP cancellation is acknowledged and ignored"
    connection.close()
    modern = request("server/discover")
    for overrides in [fields, {"Mcp-Method": None}, {"MCP-Protocol-Version": None}]:
        status, _, body = server.curl(modern, overrides)
        assert status == 400 and json.loads(body)["error"]["code"] == -32020, "legacy traffic never relaxes modern headers"
    missing = request("tools/list")
    missing["params"] = {}
    status, _, body = server.curl(missing)
    assert status == 400 and json.loads(body)["error"]["code"] == -32602
    for malformed in [b"[1,2]", b'{"jsonrpc":"2.0","id":3,"result":{}}']:
        status, _, body = server.curl(modern, raw=malformed)
        assert status == 400 and json.loads(body)["error"]["code"] == -32600
    print("PASS: stateless legacy HTTP, JSON only, ignored sessions/cancellation, strict modern isolation")


def run():
    with tempfile.TemporaryDirectory(prefix="mcp-http-") as directory:
        server = Server(sys.argv[1], directory, sys.argv[3] == "Tls")
        try:
            legacy_qualification(server)
            value = request("server/discover")
            status, head, body = server.curl(value)
            assert status == 200 and json.loads(body)["result"]["supportedVersions"][0] == "2026-07-28"
            assert b"mcp-session-id" not in head.lower()
            for origin, expected in [("http://localhost:3000", 200), ("null", 403), ("http://evil", 403), ("malformed", 403), ("", 403)]:
                actual = server.curl(value, {"Origin": origin})[0]
                assert actual == expected, (origin, actual, expected)
            status, headers, _ = server.curl(value, {"Authorization": "Bearer wrong"})
            assert status == 401 and b'WWW-Authenticate: Bearer realm="fixture"'.lower() in headers.lower()
            assert b'resource_metadata="https://example.com/.well-known/oauth-protected-resource/mcp"' in headers
            status, headers, _ = server.curl(value, {"Authorization": "Bearer insufficient"})
            assert status == 403 and b'error="insufficient_scope"' in headers and b'scope="test"' in headers
            for path in ["/.well-known/oauth-protected-resource/mcp", "/.well-known/oauth-protected-resource"]:
                original = server.url
                server.url = original[:-4] + path
                status, _, document = server.curl(overrides={"Authorization": None}, method="GET")
                server.url = original
                metadata = json.loads(document)
                assert status == 200 and metadata["resource"] == "https://example.com/mcp" and metadata["authorization_servers"] == ["https://auth.example.com"]
            status, headers, _ = server.curl(value, {"Authorization": "Bearer forbidden"})
            assert status == 403 and b"www-authenticate" not in headers.lower()
            for credential in ["Bearer malformed", "Bearer throw"]:
                assert server.curl(value, {"Authorization": credential})[0] == 500
            assert server.curl(value, {"Origin": "http://evil", "Authorization": "Bearer throw"})[0] == 403
            for credential, expected in [("Bearer fixture", {"subject": "fixture-user", "claims": {"scope": "test"}}),
                                         ("Bearer anonymous", {"anonymous": True})]:
                identity = request("tools/call", {"name": "identity"})
                status, _, body = server.curl(identity, {"Authorization": credential})
                result = json.loads(body.split(b"data: ", 1)[1])["result"]
                assert status == 200 and result["structuredContent"] == expected
                assert json.loads(result["content"][0]["text"]) == expected
            for method in ["GET", "DELETE"]:
                assert server.curl(method=method)[0] == 405
            for content_type in ["text/plain", "application/jsonx", "application/json junk", None]:
                assert server.curl(value, {"Content-Type": content_type})[0] == 415
            assert server.curl(value, {"Content-Type": "Application/JSON; charset=utf-8"})[0] == 200
            for accept in ["application/json", "text/event-stream", "application/jsonx, text/event-stream"]:
                assert server.curl(value, {"Accept": accept})[0] == 406
            status, _, body = server.curl(value, raw=b"{bad}")
            assert status == 400 and json.loads(body)["error"]["code"] == -32700
            assert server.curl(value, raw=b"[1,2]")[0] == 400
            assert server.curl(value, raw=b"x" * 1048577)[0] == 413
            for override in [{"Mcp-Method": None}, {"Mcp-Method": "wrong"}, {"MCP-Protocol-Version": None}]:
                status, _, body = server.curl(value, override)
                assert status == 400 and json.loads(body)["error"]["code"] == -32020
            no_meta = request("tools/list")
            no_meta["params"] = {}
            status, _, body = server.curl(no_meta)
            assert status == 400 and json.loads(body)["error"]["code"] == -32602
            assert server.curl(request("ping"))[0] == 404
            for name in ["echo", "écho", "=?base64?literal?="]:
                call = request("tools/call", {"name": name, "arguments": {"message": "héllo\nworld"}})
                encoded = "=?base64?" + base64.b64encode(name.encode()).decode() + "?="
                status, head, body = server.curl(call, {"Mcp-Name": encoded})
                events = [json.loads(event.split(b"data: ", 1)[1]) for event in body.split(b"\n\n") if event]
                assert status == 200 and b"text/event-stream" in head and len(events) == 1
                assert events[0]["result"]["structuredContent"] == {"pipelining": False, "pending": 1}
            call = request("tools/call", {"name": "echo"})
            status, _, body = server.curl(call, {"Mcp-Name": "=?base64?%%%?="})
            assert status == 400 and json.loads(body)["error"]["code"] == -32020
            notification = {"jsonrpc": "2.0", "method": "notifications/cancelled", "params": {"requestId": 99}}
            assert server.curl(notification)[::2] == (202, b"")
            connection = server.connection()
            connection.sendall(wire(request("tools/call", {"name": "large"})))
            head, body = read_response(connection)
            assert head.count(b"HTTP/1.1") == 1 and body.endswith(b"\n\n")
            assert len(json.loads(body.split(b"data: ", 1)[1])["result"]["content"][0]["text"]) == 200000
            connection.sendall(wire(request("tools/list", ident=2)))
            assert json.loads(read_response(connection)[1])["id"] == 2
            connection.close()
            overflow = server.connection()
            overflow.sendall(wire(request("tools/call", {"name": "overflow"})))
            data = overflow.recv(4096)
            while data:
                data = overflow.recv(4096)
            overflow.close()
            assert server.curl(value)[0] == 200, "output limit closes only the offending connection"
            pipeline = server.connection()
            pipeline.sendall(wire(value) + wire(request("tools/list", ident=2)))
            assert json.loads(read_response(pipeline)[1])["id"] == 1
            assert pipeline.recv(1) == b"", "pipelining disabled: second request is not delivered"
            pipeline.close()
            subscription = server.connection()
            subscription.sendall(wire(request("subscriptions/listen", {"notifications": {"toolsListChanged": True}}, "listen")))
            first = subscription.recv(8192)
            until = time.monotonic() + 2
            while b"notifications/subscriptions/acknowledged" not in first and time.monotonic() < until:
                first += subscription.recv(8192)
            assert b"notifications/subscriptions/acknowledged" in first
            assert server.curl(value)[0] == 200, "independent connection progresses while SSE is open"
            server.process.send_signal(signal.SIGTERM)
            remainder = b""
            data = subscription.recv(8192)
            while data:
                remainder += data
                data = subscription.recv(8192)
            assert b'"resultType":"complete"' in remainder and b"notifications/cancelled" not in remainder, (first, remainder)
            subscription.close()
            server.process.wait(timeout=4)
        finally:
            if server.process.poll() is None:
                server.stop()
            else:
                server.errors.seek(0)
                errors = server.errors.read()
                assert server.process.returncode == 241, (server.process.returncode, errors)
                assert b"AddressSanitizer" not in errors, errors.decode(errors="replace")
                server.errors.close()
        print("PASS: curl headers/Origin/auth/JSON/media/errors, SSE wire/EOF/keepalive, finite defaults, pipeline/shutdown, " + sys.argv[3])


if __name__ == "__main__":
    run()
