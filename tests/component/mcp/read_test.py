# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
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
