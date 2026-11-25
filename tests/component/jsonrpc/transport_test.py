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
import select
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import time


class Reader:
    def __init__(self, fd):
        self.fd = fd
        self.buffer = b""

    def exact(self, size):
        while len(self.buffer) < size:
            assert select.select([self.fd], [], [], 4)[0], "response timeout"
            chunk = os.read(self.fd, 65536)
            assert chunk, "unexpected EOF"
            self.buffer += chunk
        value, self.buffer = self.buffer[:size], self.buffer[size:]
        return value

    def line(self):
        data = b""
        while not data.endswith(b"\n"):
            data += self.exact(1)
        return data

    def message(self, framing):
        if framing == "newline":
            value = json.loads(self.line())
        else:
            header = b""
            while not header.endswith(b"\r\n\r\n"):
                header += self.exact(1)
            size = int(header.split(b":", 1)[1].strip())
            value = json.loads(self.exact(size))
        return value


def encode(value, framing):
    message = json.dumps(value, ensure_ascii=False, separators=(",", ":")).encode()
    frame = message + b"\n" if framing == "newline" else b"Content-Length: " + str(len(message)).encode() + b"\r\n\r\n" + message
    return frame


def frame(payload, opcode=1, final=True):
    mask = b"abcd"
    size = len(payload)
    length = bytes([size | 128]) if size < 126 else bytes([126 | 128]) + struct.pack("!H", size)
    value = bytes([opcode | (128 if final else 0)]) + length + mask + bytes(c ^ mask[i % 4] for i, c in enumerate(payload))
    return value


def receive_frame(reader):
    header = reader.exact(2)
    size = header[1] & 127
    if size == 126:
        size = struct.unpack("!H", reader.exact(2))[0]
    if size == 127:
        size = struct.unpack("!Q", reader.exact(8))[0]
    return header[0] & 15, reader.exact(size)


def connect(transport, path, port):
    connection = socket.socket(socket.AF_UNIX if transport == "unix" else socket.AF_INET, socket.SOCK_STREAM)
    connected = False
    deadline = time.monotonic() + 6
    while not connected and time.monotonic() < deadline:
        try:
            connection.connect(path if transport == "unix" else ("127.0.0.1", port))
            connected = True
        except (ConnectionRefusedError, FileNotFoundError):
            time.sleep(0.02)
    assert connected, "listener did not start"
    return connection


def rejected_inputs(framing):
    values = [b"x" * (1024 * 1024 + 1)] if framing == "newline" else [
        b"Content-Length: -1\r\n\r\n", b"Content-Length: 1x\r\n\r\n",
        b"Content-Length: 0\r\nContent-Length: 0\r\n\r\n", b"Other: 1\r\n\r\n",
        b"Content-Length: 1048577\r\n\r\n", b"x" * 8193]
    return values


def stdio_bounds(executable, framing):
    for value in rejected_inputs(framing):
        process = subprocess.run([executable, "--config-file=/dev/null", "--transport=stdio", "--framing=" + framing],
                                 input=value, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=6)
        assert process.returncode == 0 and not process.stdout, (process.returncode, process.stdout, process.stderr)
        assert b"invalid JSON-RPC stream framing" in process.stderr and b"AddressSanitizer" not in process.stderr


def run(executable, transport, framing):
    with tempfile.TemporaryDirectory(prefix="jsonrpc-") as directory:
        with socket.socket() as probe:
            probe.bind(("127.0.0.1", 0))
            port = probe.getsockname()[1]
        path = os.path.join(directory, "rpc.sock")
        with tempfile.TemporaryFile() as diagnostics:
            process = subprocess.Popen([executable, "--config-file=/dev/null", "--transport=" + transport,
                                        "--framing=" + framing, "--port=" + str(port), "--path=" + path],
                                       stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=diagnostics, cwd=directory)
            connection = None
            try:
                if transport == "stdio":
                    write = lambda data: (process.stdin.write(data), process.stdin.flush())
                    reader = Reader(process.stdout.fileno())
                else:
                    connection = connect(transport, path, port)
                    write = connection.sendall
                    reader = Reader(connection.fileno())
                request = {"jsonrpc": "2.0", "method": "add", "params": [20, 22], "id": "é日"}
                if transport == "websocket":
                    key = base64.b64encode(b"0123456789abcdef").decode()
                    write(("GET /ws HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                           "Sec-WebSocket-Version: 13\r\nSec-WebSocket-Protocol: jsonrpc\r\nSec-WebSocket-Key: " + key + "\r\n\r\n").encode())
                    header = b""
                    while not header.endswith(b"\r\n\r\n"):
                        header += reader.exact(1)
                    assert b"101" in header and b"jsonrpc" in header, header
                    payload = json.dumps(request, ensure_ascii=False).encode()
                    write(frame(payload[:9], final=False) + frame(payload[9:], opcode=0))
                    opcode, value = receive_frame(reader)
                    assert opcode == 1 and json.loads(value) == {"jsonrpc": "2.0", "id": "é日", "result": 42.0}, value
                    client = subprocess.run([executable, "--config-file=/dev/null", "--transport=websocket-client", "--port=" + str(port)],
                                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=8)
                    assert client.returncode == 0 and all(word in client.stderr for word in [b"calculator result:", b"calculator error:", b"calculator timeout:"]), client.stderr
                    write(frame(b"binary", opcode=2))
                    opcode, value = receive_frame(reader)
                    assert opcode == 8 and struct.unpack("!H", value[:2])[0] == 1003, (opcode, value)
                else:
                    payload = encode(request, framing)
                    for part in [payload[:3], payload[3:11], payload[11:]]:
                        write(part)
                        time.sleep(0.005)
                    assert reader.message(framing) == {"jsonrpc": "2.0", "id": "é日", "result": 42.0}
                    notification = {"jsonrpc": "2.0", "method": "add", "params": [1, 2]}
                    requests = [{**request, "id": index} for index in range(12)]
                    write(encode(notification, framing) + b"".join(encode(value, framing) for value in requests))
                    for index in range(12):
                        assert reader.message(framing) == {"jsonrpc": "2.0", "id": index, "result": 42.0}
                    if transport == "tcp" and framing == "newline":
                        client = subprocess.run([executable, "--config-file=/dev/null", "--transport=client", "--port=" + str(port)],
                                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=8)
                        assert client.returncode == 0 and all(word in client.stderr for word in [b"calculator result:", b"calculator error:", b"calculator timeout:"]), client.stderr
                    write(b'{"jsonrpc":' if framing == "newline" else b'Content-Length: 100\r\n\r\n{"')
                    if transport == "stdio":
                        process.stdin.close()
                    else:
                        connection.shutdown(socket.SHUT_WR)
                    response = reader.message(framing)
                    assert response == {"jsonrpc": "2.0", "id": None, "error": {"code": -32700, "message": "Parse error"}}, response
                if connection:
                    connection.close()
                    connection = None
                if transport == "unix":
                    for value in rejected_inputs(framing):
                        with connect(transport, path, port) as rejected:
                            rejected.sendall(value)
                            assert select.select([rejected], [], [], 4)[0], "invalid framing was not closed"
                            assert rejected.recv(1) == b"", "invalid framing produced output"
                if transport == "stdio":
                    assert process.wait(timeout=6) == 0
                else:
                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=6)
                diagnostics.seek(0)
                assert b"AddressSanitizer" not in diagnostics.read()
            finally:
                if connection:
                    connection.close()
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=5)
    if transport == "stdio":
        stdio_bounds(executable, framing)
    print("PASS: calculator", transport, framing)


if __name__ == "__main__":
    run(*sys.argv[1:])
