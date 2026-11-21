# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from http_test import Server


def run():
    node, sdk, stdio, http, curl, root = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="mcp-sdk-read-", dir=os.environ["TMPDIR"]) as directory:
        server = Server(http, directory, False, ["--header-root", root], curl=curl)
        results = []
        try:
            for kind, target in [("stdio", stdio), ("http", server.url)]:
                result = subprocess.run([node, str(Path(__file__).with_name("read_sdk.mjs")), sdk, kind, target, root],
                                        capture_output=True, timeout=15)
                print((result.stdout + result.stderr).decode(), flush=True)
                results.append(result.returncode)
        finally:
            server.stop()
        assert results == [0, 0], results


run()
