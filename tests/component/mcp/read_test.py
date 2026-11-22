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
from pathlib import Path
import sys
import tempfile

from stdio_test import Client, request
from http_test import Server, request as http_request


def verify(result, root, arguments):
    offset = arguments.get("offset", 0)
    expected = (root / arguments["path"]).read_bytes()[offset:offset + arguments.get("bytes", 16384)]
    value = result["structuredContent"]
    assert value["text"].encode() == expected, "model-visible structuredContent must contain the exact requested bytes"
    assert value["path"] == arguments["path"] and value["offset"] == offset
    assert value["bytes"] == len(expected)
    assert value["truncated"] == (offset + len(expected) < (root / arguments["path"]).stat().st_size)
    assert json.loads(result["content"][0]["text"]) == value, "both MCP representations carry the same complete result"


def run():
    root = Path(sys.argv[4])
    cases = [{"path": "core/SNodeC.h", **arguments} for arguments in [
        {"offset": 2000, "bytes": 2500}, {"offset": 2220, "bytes": 1500}, {},
        {"offset": 2368, "bytes": 200}, {"offset": 10000, "bytes": 200}]]
    with tempfile.TemporaryDirectory(prefix="mcp-read-") as directory:
        client = Client(sys.argv[1], ["--config-file", "/dev/null", "mcp", "--header-root", str(root)], directory)
        server = Server(sys.argv[2], directory, False, ["--header-root", str(root)], curl=sys.argv[3])
        try:
            for arguments in cases:
                client.send(request("tools/call", 1, {"name": "api.read", "arguments": arguments}))
                verify(client.receive()["result"], root, arguments)
                status, _, body = server.curl(http_request("tools/call", {"name": "api.read", "arguments": arguments}))
                assert status == 200
                verify(json.loads(body.split(b"data: ", 1)[1])["result"], root, arguments)
            print("PASS: stdio and HTTP api.read preserve exact source bytes in both result representations")
        finally:
            client.finish()
            server.stop()


if __name__ == "__main__":
    run()
