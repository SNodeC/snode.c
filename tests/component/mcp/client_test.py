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

"""Exercise the installed-style client API through its CLI and real transports."""
import json
import os
import subprocess
import sys
import tempfile
import http_test
import socket
import select
import threading

client, stdio, http = sys.argv[1:4]
flow = sys.argv[4] if len(sys.argv) > 4 else None

def run(directory, transport, legacy, command="tools", name="", arguments=None):
    argv = [client, "--config-file", "/dev/null", "--log-console", "stderr", "--quiet",
            "--command", command, "--name", name, "--arguments", json.dumps(arguments or {}),
            "--legacy", str(legacy).lower()]
    if isinstance(transport, str):
        argv += ["--server", transport, "--server-arguments", json.dumps(["--config-file", "/dev/null", "--log-console", "stderr", "mcp", "--page-size", "1"])]
    else:
        argv += ["--port", str(transport.port)]
    result = subprocess.run(argv, capture_output=True, timeout=12,
                            env=dict(os.environ, XDG_CONFIG_HOME=directory, XDG_STATE_HOME=directory, MCP_BEARER_TOKEN="fixture"))
    assert result.returncode == 0, (argv, result.returncode, result.stdout.decode(), result.stderr.decode())
    lines = result.stdout.decode().splitlines()
    values = [json.loads(line) for line in lines if line.startswith('{')]
    assert values, (result.stdout, result.stderr)
    assert ("2025-11-25" if legacy else "2026-07-28") in result.stderr.decode()
    return values[-1]

class DisconnectProxy:
    def __init__(self, backend):
        self.backend = backend
        self.listener = socket.socket()
        self.listener.bind(("127.0.0.1", 0))
        self.listener.listen()
        self.listener.settimeout(0.1)
        self.port = self.listener.getsockname()[1]
        self.stopped = False
        self.dropped = False
        self.resumed = False
        self.handlers = []
        self.thread = threading.Thread(target=self.accept)
        self.thread.start()

    def accept(self):
        while not self.stopped:
            try:
                connection, _ = self.listener.accept()
            except (TimeoutError, OSError):
                connection = None
            if connection is not None:
                handler = threading.Thread(target=self.pump, args=(connection,))
                self.handlers.append(handler)
                handler.start()

    def pump(self, downstream):
        upstream = socket.create_connection(("127.0.0.1", self.backend), timeout=2)
        upstream.settimeout(None)
        request = b""
        response = b""
        pumping = True
        try:
            while pumping and not self.stopped:
                readable, _, _ = select.select([downstream, upstream], [], [], 0.1)
                for source in readable:
                    chunk = source.recv(65536)
                    if not chunk:
                        pumping = False
                    else:
                        destination = upstream if source is downstream else downstream
                        destination.sendall(chunk)
                        if source is downstream:
                            request += chunk
                            if b"last-event-id:" in request.lower():
                                self.resumed = True
                        else:
                            response += chunk
                            if (not self.dropped and b'"name":"delay"' in request and
                                    b"id: " in response and b"data:\n\n" in response):
                                self.dropped = True
                                pumping = False
        except (OSError, TimeoutError):
            pass
        finally:
            downstream.close()
            upstream.close()

    def stop(self):
        self.stopped = True
        self.listener.close()
        self.thread.join(timeout=2)
        for handler in self.handlers:
            handler.join(timeout=3)

with tempfile.TemporaryDirectory() as directory:
    server = http_test.Server(http, directory, False, extra=("--page-size", "1"), curl="curl")
    try:
        for transport in (stdio, server):
            for legacy in (False, True):
                tools = run(directory, transport, legacy)["tools"]
                assert {item["name"] for item in tools} >= {"echo", "delay", "ask"}
                echo = run(directory, transport, legacy, "call", "echo", {"value": 42})
                if isinstance(transport, str):
                    assert echo["content"][0]["text"] == '{"value":42}'
                else:
                    assert echo["structuredContent"] == {"pipelining": False, "pending": 1}
                assert run(directory, transport, legacy, "call", "delay")["content"] == []
                assert run(directory, transport, legacy, "read", "test://updates")["contents"][0]["text"] == "first"
                assert run(directory, transport, legacy, "prompt", "ask")["messages"] == 1
                assert run(directory, transport, legacy, "call", "ask")["content"][0]["text"] == "opaque"
                assert run(directory, transport, legacy, "templates")["resourceTemplates"]
                assert run(directory, transport, legacy, "resources")["resources"]
                assert run(directory, transport, legacy, "prompts")["prompts"]
        if flow:
            for legacy in (False, True):
                for transport in (stdio, server):
                    command = [flow, "--config-file", "/dev/null", "--quiet", "--log-console", "stderr", "--legacy", str(legacy).lower()]
                    command += ["--server", transport] if isinstance(transport, str) else ["--port", str(transport.port)]
                    result = subprocess.run(command, capture_output=True, timeout=12, env=dict(os.environ, XDG_CONFIG_HOME=directory, XDG_STATE_HOME=directory))
                    assert result.returncode == 0, (command, result.stdout.decode(), result.stderr.decode())
        proxy = DisconnectProxy(server.port)
        try:
            result = run(directory, proxy, True, "call", "delay", {"reportCount": True})
            assert result["structuredContent"]["calls"] == 3, result
            assert proxy.dropped and proxy.resumed, "SSE disconnection resumes the same stream without resubmitting POST"
        finally:
            proxy.stop()
    finally:
        server.stop()
print("SNode.C client/server: both eras, stdio and HTTP, lists/call/async/resource/prompt/MRTR/elicitation and forced-disconnect replay passed")
