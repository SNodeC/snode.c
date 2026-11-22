# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
import sys
import tempfile
import time
from stdio_test import Client, request

with tempfile.TemporaryDirectory(prefix="mcp-idle-") as directory:
    client = Client(sys.argv[1], ["--config-file", "/dev/null", "--quiet"], directory)
    client.send(request("server/discover", 1))
    assert client.receive()["id"] == 1
    time.sleep(61)
    client.send(request("server/discover", 2))
    assert client.receive()["id"] == 2, "MCP idle connection must survive the core's 60-second default"
    client.finish()
    print("PASS: real 61-second idle interval without transport polling")
