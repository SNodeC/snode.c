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

import json
import os
import select
import signal
import subprocess
import sys
import tempfile
import time

MODERN = "2026-07-28"
META = {"io.modelcontextprotocol/protocolVersion": MODERN, "io.modelcontextprotocol/clientCapabilities": {}}


def request(method, ident, params=None, modern=True):
    args = dict(params or {})
    if modern:
        args["_meta"] = META
    return {"jsonrpc": "2.0", "id": ident, "method": method, "params": args}


class Client:
    def __init__(self, executable, args, directory):
        environment = dict(os.environ, XDG_CONFIG_HOME=directory, XDG_STATE_HOME=directory)
        self.errors = tempfile.TemporaryFile()
        self.process = subprocess.Popen([executable] + args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=self.errors, env=environment, bufsize=0)
        self.buffer = b""
        self.messages = []

    def send(self, value, split=False):
        wire = json.dumps(value, ensure_ascii=False).encode() + b"\n"
        if split:
            self.process.stdin.write(wire[:5])
            time.sleep(0.01)
            self.process.stdin.write(wire[5:])
        else:
            self.process.stdin.write(wire)

    def receive(self, timeout=4):
        until = time.monotonic() + timeout
        while b"\n" not in self.buffer and time.monotonic() < until:
            ready, _, _ = select.select([self.process.stdout], [], [], 0.1)
            if ready:
                data = os.read(self.process.stdout.fileno(), 65536)
                assert data, "unexpected stdout EOF"
                self.buffer += data
        assert b"\n" in self.buffer, "protocol response timed out"
        line, self.buffer = self.buffer.split(b"\n", 1)
        message = json.loads(line)
        self.messages.append(message)
        return message

    def finish(self, sig=None):
        if sig is not None:
            self.process.send_signal(sig)
        self.process.stdin.close()
        rest = self.process.stdout.read()
        self.process.wait(timeout=4)
        for line in (self.buffer + rest).splitlines():
            self.messages.append(json.loads(line))
        self.errors.seek(0)
        errors = self.errors.read()
        assert b"AddressSanitizer" not in errors, errors.decode(errors="replace")
        assert self.process.returncode == (0 if sig is None else (256 - sig)), (self.process.returncode, errors)
        self.errors.close()
        return errors


def run(executable):
    with tempfile.TemporaryDirectory(prefix="mcp-stdio-") as directory:
        for flags in [[], ["--quiet"], ["--log-console", "stderr"]]:
            client = Client(executable, flags, directory)
            client.send(request("server/discover", 1), split=True)
            assert client.receive()["result"]["supportedVersions"][0] == MODERN
            client.send(request("tools/call", 2, {"name": "echo", "arguments": {"text": "é日\nvalue"}}))
            assert "é日" in client.receive()["result"]["content"][0]["text"]
            client.process.stdin.write(b"{broken}\nnull\n")
            assert client.receive()["error"]["code"] == -32700
            assert client.receive()["error"]["code"] == -32600
            client.send(request("tools/call", 3, {"name": "delay"}))
            client.send(request("tools/call", 3, {"name": "delay"}))
            duplicate = client.receive()
            assert duplicate == {"jsonrpc": "2.0", "id": 3, "error": {"code": -32600, "message": "Duplicate id or in-flight limit"}}
            client.send({"jsonrpc": "2.0", "method": "notifications/cancelled", "params": {"requestId": 3}})
            client.send(request("tools/list", 4))
            assert client.receive()["id"] == 4
            time.sleep(0.25)
            errors = client.finish()
            assert all(message.get("id") != 3 or message == duplicate for message in client.messages)
            if not flags:
                assert b"MCP fixture ready" in errors, errors
        batch = Client(executable, ["--quiet"], directory)
        batch.process.stdin.write(b"".join(json.dumps(request("tools/list", i)).encode() + b"\n" for i in range(100)))
        batch.finish()
        assert len(batch.messages) == 100
        for modern in [True, False]:
            partial = Client(executable, [], directory)
            expected = []
            if not modern:
                partial.send(request("initialize", 20, {"protocolVersion": "2025-11-25", "capabilities": {},
                                                        "clientInfo": {"name": "test", "version": "1"}}, False))
                initialized = partial.receive()
                assert initialized["id"] == 20 and initialized["result"]["protocolVersion"] == "2025-11-25"
                expected.append(initialized)
            partial.process.stdin.write(b'{"jsonrpc":"2.0","id":99')
            partial.finish()
            error = {"jsonrpc": "2.0", "error": {"code": -32700, "message": "Parse error"}}
            if not modern:
                error["id"] = None
            expected.append(error)
            assert partial.messages == expected, partial.messages
        pressure = Client(executable, ["mcp", "--maximum-output-bytes", "1024"], directory)
        pressure.send(request("tools/call", 6, {"name": "echo", "arguments": {"value": "x" * 4096}}))
        assert b"stdio output limit exceeded" in pressure.finish()
        assert not pressure.messages, "oversized output closes without partial protocol frames"
        legacy = Client(executable, [], directory)
        legacy.send(request("initialize", 8, {"protocolVersion": "2025-11-25"}, False))
        assert legacy.receive()["error"]["code"] == -32602
        legacy.send(request("tools/list", 9, modern=False))
        assert legacy.receive()["error"]["code"] == -32602, "failed initialize cannot establish legacy context"
        legacy.send(request("initialize", 10, {"protocolVersion": "2025-06-18", "capabilities": {},
                                               "clientInfo": {"name": "test", "version": "1"}}, False))
        assert legacy.receive()["result"]["protocolVersion"] == "2025-11-25"
        legacy.send({"jsonrpc": "2.0", "method": "notifications/initialized"})
        legacy.send(request("tools/call", 11, {"name": "delay"}, False))
        legacy.send(request("tools/call", 11, {"name": "delay"}, False))
        duplicate = legacy.receive()
        assert duplicate == {"jsonrpc": "2.0", "id": 11, "error": {"code": -32600, "message": "Duplicate id or in-flight limit"}}
        legacy.send({"jsonrpc": "2.0", "method": "notifications/cancelled", "params": {"requestId": 11}})
        legacy.send(request("ping", 12, modern=False))
        assert legacy.receive() == {"jsonrpc": "2.0", "id": 12, "result": {}}
        legacy.send(request("tools/list", 13))
        assert legacy.receive()["result"]["resultType"] == "complete"
        time.sleep(0.25)
        legacy.finish()
        assert all(message.get("id") != 11 or message == duplicate for message in legacy.messages)
        subscription = Client(executable, [], directory)
        subscription.send(request("subscriptions/listen", "listen"))
        assert subscription.receive()["method"] == "notifications/subscriptions/acknowledged"
        subscription.finish(signal.SIGTERM)
        assert any(message.get("method") == "notifications/cancelled" for message in subscription.messages)
        requested = Client(executable, ["mcp", "--fixture-schema", "stop"], directory)
        requested.send(request("subscriptions/listen", "requested-listen"))
        assert requested.receive()["method"] == "notifications/subscriptions/acknowledged"
        requested.send(request("tools/call", "stop", {"name": "delay"}))
        cancelled = requested.receive()
        assert cancelled["method"] == "notifications/cancelled" and cancelled["params"]["requestId"] == "requested-listen"
        requested.finish()
        for flags in [["--bad-option"], ["--quiet", "--bad-option"], ["--log-console", "bad"], ["--help"]]:
            result = subprocess.run([executable] + flags, input=b"", stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                    env=dict(os.environ, XDG_CONFIG_HOME=directory), timeout=4)
            assert result.stdout == b"", result.stdout
            assert result.stderr, flags
            assert result.returncode == (1 if flags == ["--log-console", "bad"] else 2), (flags, result.returncode)
        invalid = subprocess.run([executable, "mcp", "--fixture-schema", "invalid"], input=b"", capture_output=True,
                                 env=dict(os.environ, XDG_CONFIG_HOME=directory), timeout=4)
        assert invalid.returncode == 1 and invalid.stdout == b""
        assert b"registration rejected" in invalid.stderr, invalid.stderr
        for kind in ["throw", "throw-int"]:
            failed = subprocess.run([executable, "--config-file", "/dev/null", "mcp", "--fixture-schema", kind], input=b"", capture_output=True, timeout=4)
            assert failed.returncode == 1 and failed.stdout == b"" and failed.stderr, (kind, failed)
            assert b"AddressSanitizer" not in failed.stderr
        print("PASS: stdio framing, eras, cancellation, EOF/signal, logging/quiet/CLI stdout purity")


if __name__ == "__main__":
    run(sys.argv[1])
