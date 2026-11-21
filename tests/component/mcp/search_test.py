# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time

from stdio_test import Client, request
from http_test import Server, headers, request as http_request, wire

# Per-transport budgets leave ample room for sanitizer builds while remaining
# far below the unchanged 30-second server deadline.
BUDGET = {"stdio": 8, "http": 8}
RESPONSIVENESS = 0.75
QUERY = "static void init("


def verify(result, root):
    assert not result.get("isError"), result
    value = result["structuredContent"]
    expected = []
    files = list(root.rglob("*.h")) + list(root.rglob("*.hpp"))
    for path in files:
        for number, line in enumerate(path.read_text().splitlines(), 1):
            if QUERY in line:
                expected.append((str(path.relative_to(root)), number))
    assert sorted((match["path"], match["line"]) for match in value["matches"]) == sorted(expected)
    assert value["files"] == len(files) and value["bytes"] == sum(path.stat().st_size for path in files)
    assert value["files"] >= 500 and value["bytes"] >= 2500000, "exercise the source-sized tree, not staged installed headers"
    assert not value["truncated"]
    print(json.dumps(value), flush=True)


def stdio(executable, directory, root, listening):
    client = Client(executable, ["--config-file", "/dev/null", "mcp", "--header-root", str(root)], directory)
    try:
        if listening:
            client.send(request("subscriptions/listen", "listen"))
            assert client.receive()["method"] == "notifications/subscriptions/acknowledged"
        started = time.monotonic()
        client.send(request("tools/call", 50, {"name": "api.find", "arguments": {"query": QUERY}}))
        time.sleep(0.05)
        probe = time.monotonic()
        client.send(request("tools/call", 51, {"name": "api.read", "arguments": {"path": "core/SNodeC.h", "offset": 2368, "bytes": 200}}))
        fast = client.receive()
        latency = time.monotonic() - probe
        assert fast["id"] == 51 and "static void init(" in fast["result"]["structuredContent"]["text"], fast
        assert latency < RESPONSIVENESS, latency
        response = client.receive(timeout=BUDGET["stdio"])
        elapsed = time.monotonic() - started
        assert response["id"] == 50 and elapsed < BUDGET["stdio"], (elapsed, response)
        verify(response["result"], root)
        print(f"PASS: stdio listen={listening} search={elapsed:.3f}s concurrent-read={latency:.3f}s", flush=True)
    finally:
        client.finish(signal.SIGTERM if listening else None)


def http(executable, curl, directory, root, listening):
    server = Server(executable, directory, False, ["--header-root", str(root)], curl=curl)
    stream = None
    search = None
    try:
        if listening:
            stream = server.connection()
            stream.sendall(wire(http_request("subscriptions/listen", ident="listen")))
            received = b""
            while b"notifications/subscriptions/acknowledged" not in received:
                received += stream.recv(65536)
        value = http_request("tools/call", {"name": "api.find", "arguments": {"query": QUERY}}, ident=50)
        command = [curl, "-sS", "--max-time", str(BUDGET["http"]), "--http1.1"]
        for key, text in headers(value).items():
            command += ["-H", key + ": " + text]
        started = time.monotonic()
        search = subprocess.Popen(command + ["--data-binary", json.dumps(value), server.url], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        time.sleep(0.05)
        assert search.poll() is None, "long search remains in progress during concurrent request"
        probe = time.monotonic()
        status, _, body = server.curl(http_request("tools/call", {"name": "api.read", "arguments": {"path": "core/SNodeC.h", "offset": 2368, "bytes": 200}}))
        latency = time.monotonic() - probe
        assert status == 200 and "static void init(" in json.loads(body.split(b"data: ", 1)[1])["result"]["structuredContent"]["text"]
        assert latency < RESPONSIVENESS and search.poll() is None, latency
        output, error = search.communicate(timeout=BUDGET["http"] + 1)
        elapsed = time.monotonic() - started
        assert search.returncode == 0 and elapsed < BUDGET["http"], (elapsed, error)
        verify(json.loads(output.split(b"data: ", 1)[1])["result"], root)
        print(f"PASS: HTTP listen={listening} search={elapsed:.3f}s concurrent-read={latency:.3f}s", flush=True)
    finally:
        if search is not None and search.poll() is None:
            search.kill()
            search.communicate()
        if stream is not None:
            stream.close()
        server.stop()


def run():
    kind, executable, curl, source = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="mcp-search-", dir=os.environ["TMPDIR"]) as directory:
        failures = []
        for listening in [False, True]:
            try:
                if kind == "stdio":
                    stdio(executable, directory, Path(source), listening)
                else:
                    http(executable, curl, directory, Path(source), listening)
            except Exception as error:
                print(f"FAIL: {kind} listen={listening}: {error}", flush=True)
                failures.append(str(error))
        assert not failures, failures


run()
