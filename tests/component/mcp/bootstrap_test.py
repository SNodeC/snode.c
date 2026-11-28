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
            assert shared.returncode == 0 and any(line.startswith(b'{') for line in shared.stdout.splitlines()), shared
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
        child = subprocess.Popen([demo, "--config-file", "/dev/null"] + demo_args,
                                 stdin=slave, stdout=slave, stderr=slave, env=env)
        os.close(slave)
        try:
            os.write(master, wire)
            data = b""
            until = time.monotonic() + 5
            while not any(line.startswith(b'{') and line.endswith(b"}") for line in data.splitlines()) and time.monotonic() < until:
                if select.select([master], [], [], 0.1)[0]:
                    data += os.read(master, 65536)
            assert any(json.loads(line)["id"] == 1 for line in data.splitlines() if line.startswith(b'{')), data
            child.send_signal(signal.SIGTERM)
            assert child.wait(timeout=5) == 241
        finally:
            os.close(master)
            if child.poll() is None:
                child.kill()
                child.wait()
        print("PASS: stdout retention, silent daemon refusal, bootstrap reparse cleanup, shared console pipes and non-quiet tty")


if __name__ == "__main__":
    run(*sys.argv[1:])
