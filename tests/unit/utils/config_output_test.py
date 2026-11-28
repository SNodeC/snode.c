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

import errno
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import tty


def run(executable):
    with tempfile.TemporaryDirectory(prefix="config-output-") as directory:
        env = dict(os.environ, XDG_CONFIG_HOME=directory, XDG_STATE_HOME=directory, XDG_RUNTIME_DIR=directory)
        command = [executable, "--config-file", "/dev/null"]
        cases = [
            ("help", ["--help"], True, b"Usage:"),
            ("version", ["--version"], True, b"."),
            ("config", ["-s"], True, b"log-console"),
            ("command", ["--command-line"], True, b"command@line"),
            ("write", ["-w", directory + "/success.conf"], True, b"SUCCESS"),
            ("write-error", ["-w", directory + "/absent/x.conf"], False, b"Writing config file"),
            ("write-validation", ["-w", directory], False, b"Error"),
            ("invalid", ["--bogus"], False, b"bogus"),
            ("unknown", ["unknown-command"], False, b"unknown-command"),
        ]
        for name, arguments, normal, marker in cases:
            result = subprocess.run(command + arguments, capture_output=True, env=env, timeout=5)
            # Initial validation exits 1; the existing CLI-only paths exit 2, including success.
            assert result.returncode == (1 if name == "write-validation" else 2), (name, result)
            selected = result.stdout if normal else result.stderr
            other = result.stderr if normal else result.stdout
            assert marker in selected and not other, (name, result)
            if not normal:
                assert b"\x1b" not in selected, (name, result)
        assert Path(directory, "success.conf").read_text(), "configuration file was not written"
        forced = subprocess.run(command + ["--monochrom=false", "--log-console", "stdout", "--bogus"],
                                capture_output=True, env=env, timeout=5)
        assert forced.returncode == 2 and not forced.stdout and b"bogus" in forced.stderr, forced
        assert b"\x1b" not in forced.stderr, forced
        master, slave = pty.openpty()
        tty.setraw(slave)
        try:
            terminal = subprocess.run(command + ["--monochrom=false", "--bogus"],
                                      stdout=subprocess.PIPE, stderr=slave, env=env, timeout=5)
            os.close(slave)
            slave = -1
            data = b""
            ended = False
            while not ended:
                try:
                    chunk = os.read(master, 65536)
                    data += chunk
                    ended = not chunk
                except OSError as error:
                    assert error.errno == errno.EIO, error
                    ended = True
            assert terminal.returncode == 2 and not terminal.stdout and b"\x1b[31m" in data, (terminal, data)
        finally:
            if slave >= 0:
                os.close(slave)
            os.close(master)
        print("PASS: requested stdout, diagnostic stderr, unchanged exits, stderr-specific color and write-config outcomes")


if __name__ == "__main__":
    run(sys.argv[1])
