# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
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

    def receive(self):
        until = time.monotonic() + 4
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
    with tempfile.TemporaryDirectory(prefix="mcp-stdio-", dir=os.environ["TMPDIR"]) as directory:
        for flags in [[], ["--quiet"], ["--log-console", "stderr"], ["--daemonize"]]:
            client = Client(executable, flags, directory)
            client.send(request("server/discover", 1), split=True)
            assert client.receive()["result"]["supportedVersions"][0] == MODERN
            client.send(request("tools/call", 2, {"name": "echo", "arguments": {"text": "é日\nvalue"}}))
            assert "é日" in client.receive()["result"]["content"][0]["text"]
            client.process.stdin.write(b"{broken}\nnull\n")
            assert client.receive()["error"]["code"] == -32700
            assert client.receive()["error"]["code"] == -32600
            client.send(request("tools/call", 3, {"name": "delay"}))
            client.send({"jsonrpc": "2.0", "method": "notifications/cancelled", "params": {"requestId": 3}})
            client.send(request("tools/list", 4))
            assert client.receive()["id"] == 4
            time.sleep(0.25)
            errors = client.finish()
            assert all(message.get("id") != 3 for message in client.messages)
            if not flags:
                assert b"MCP fixture ready" in errors, errors
        batch = Client(executable, ["--quiet"], directory)
        batch.process.stdin.write(b"".join(json.dumps(request("tools/list", i)).encode() + b"\n" for i in range(100)))
        batch.finish()
        assert len(batch.messages) == 100
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
        legacy.send({"jsonrpc": "2.0", "method": "notifications/cancelled", "params": {"requestId": 11}})
        legacy.send(request("ping", 12, modern=False))
        assert legacy.receive() == {"jsonrpc": "2.0", "id": 12, "result": {}}
        legacy.send(request("tools/list", 13))
        assert legacy.receive()["result"]["resultType"] == "complete"
        time.sleep(0.25)
        legacy.finish()
        assert all(message.get("id") != 11 for message in legacy.messages)
        subscription = Client(executable, [], directory)
        subscription.send(request("subscriptions/listen", "listen"))
        assert subscription.receive()["method"] == "notifications/subscriptions/acknowledged"
        subscription.finish(signal.SIGTERM)
        assert any(message.get("method") == "notifications/cancelled" for message in subscription.messages)
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
        print("PASS: stdio framing, eras, cancellation, EOF/signal, logging/quiet/CLI stdout purity")


if __name__ == "__main__":
    run(sys.argv[1])
