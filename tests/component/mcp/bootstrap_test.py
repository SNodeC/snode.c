# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
import json
import os
from pathlib import Path
import pty
import select
import signal
import subprocess
import sys
import tempfile
import time
import tty

from stdio_test import Client, request


def run(fixture, demo, headers):
    with tempfile.TemporaryDirectory(prefix="mcp-bootstrap-") as directory:
        env = dict(os.environ, XDG_CONFIG_HOME=directory, XDG_STATE_HOME=directory)
        wire = json.dumps(request("server/discover", 1)).encode() + b"\n"
        demo_args = ["mcp", "--header-root", headers]
        for executable, args in [(fixture, []), (demo, demo_args)]:
            command = [executable, "--config-file", "/dev/null"]
            refused = subprocess.run(command + ["--daemonize=true"] + args, input=wire, capture_output=True, env=env, timeout=5)
            assert refused.returncode == 1 and refused.stdout == b"" and refused.stderr == b"", refused
            shared = subprocess.run(command + args, input=wire, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env, timeout=5)
            assert shared.returncode == 1 and shared.stdout == b"", shared
        quiet = subprocess.run([demo, "--config-file", "/dev/null", "--quiet"] + demo_args,
                               input=wire, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env, timeout=5)
        assert quiet.returncode == 0 and json.loads(quiet.stdout)["id"] == 1, quiet
        retained = Client(fixture, ["--config-file", "/dev/null"], directory)
        retained.send(request("server/discover", 2))
        assert retained.receive()["id"] == 2
        diagnostics = retained.finish()
        assert b"fixture direct stdout diagnostic" in diagnostics and b"MCP fixture ready" in diagnostics, diagnostics
        shown = subprocess.run([fixture, "--config-file", "/dev/null", "--show-config"], input=b"", capture_output=True, env=env, timeout=5)
        assert shown.stdout == b"" and shown.stderr, shown
        path = Path(directory) / "daemon.conf"
        path.write_text("daemonize=false\n")
        reparsed = subprocess.run([fixture, "--config-file", str(path), "mcp", "--fixture-reparse-config", str(path)],
                                 input=wire, capture_output=True, env=env, timeout=5)
        assert reparsed.returncode == 2 and reparsed.stdout == b"", reparsed
        assert b"daemonize" in reparsed.stderr and b"AddressSanitizer" not in reparsed.stderr, reparsed.stderr
        master, slave = pty.openpty()
        tty.setraw(slave)
        child = subprocess.Popen([demo, "--config-file", "/dev/null", "--quiet"] + demo_args,
                                 stdin=slave, stdout=slave, stderr=slave, env=env)
        os.close(slave)
        try:
            os.write(master, wire)
            data = b""
            until = time.monotonic() + 5
            while b"\n" not in data and time.monotonic() < until:
                if select.select([master], [], [], 0.1)[0]:
                    data += os.read(master, 65536)
            assert json.loads(data)["id"] == 1, data
            child.send_signal(signal.SIGTERM)
            assert child.wait(timeout=5) == 241
        finally:
            os.close(master)
            if child.poll() is None:
                child.kill()
                child.wait()
        print("PASS: stdout retention, silent daemon/shared-console refusal, bootstrap reparse cleanup, shared quiet pipes and tty")


if __name__ == "__main__":
    run(*sys.argv[1:])
