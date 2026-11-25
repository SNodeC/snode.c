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

"""SNode.C client against the official SDK server, both revisions/transports."""
import json
import os
from pathlib import Path
import signal
import socket
import subprocess
import sys
import tempfile
import time

node, root, client = sys.argv[1:4]
fixture = str(Path(__file__).with_name('sdk_server.mjs'))
with tempfile.TemporaryDirectory() as directory:
    probe = socket.socket()
    probe.bind(('127.0.0.1', 0))
    port = probe.getsockname()[1]
    probe.close()
    environment = dict(os.environ, XDG_CONFIG_HOME=directory, XDG_STATE_HOME=directory)
    with tempfile.TemporaryFile() as diagnostics:
        server = subprocess.Popen([node, fixture, root, 'http', str(port)], stdout=diagnostics, stderr=diagnostics, env=environment)
        try:
            connected = False
            until = time.monotonic() + 5
            while not connected and time.monotonic() < until and server.poll() is None:
                try:
                    socket.create_connection(('127.0.0.1', port), timeout=0.1).close()
                    connected = True
                except OSError:
                    time.sleep(0.02)
            if not connected:
                diagnostics.seek(0)
                raise AssertionError(diagnostics.read().decode())
            for legacy in (False, True):
                for transport in ('stdio', 'http'):
                    for command, name, arguments in [('tools', '', {}), ('call', 'echo', {'value':'SDK echo'}),
                                                       ('resources', '', {}), ('read', 'sdk://resource', {}),
                                                       ('prompts', '', {}), ('prompt', 'prompt', {})]:
                        argv = [client, '--config-file', '/dev/null', '--quiet', '--log-console', 'stderr',
                                '--legacy', str(legacy).lower(), '--command', command, '--name', name, '--arguments', json.dumps(arguments)]
                        if transport == 'stdio':
                            argv += ['--server', node, '--server-arguments', json.dumps([fixture, root, 'stdio'])]
                        else:
                            argv += ['--port', str(port)]
                        result = subprocess.run(argv, capture_output=True, timeout=12, env=environment)
                        if result.returncode != 0:
                            diagnostics.seek(0)
                            raise AssertionError((legacy, transport, command, result.stdout.decode(), result.stderr.decode(), diagnostics.read().decode()))
                        value = json.loads([line for line in result.stdout.decode().splitlines() if line.startswith('{')][-1])
                        if command == 'call':
                            assert value['content'][0]['text'] == 'SDK echo', value
                        elif command == 'read':
                            assert value['contents'][0]['text'] == 'SDK resource', value
                        elif command == 'prompt':
                            assert value['messages'] == 1, value
                        else:
                            assert value[command], value
                        print(('2025' if legacy else '2026'), transport, command, 'PASS', flush=True)
        finally:
            server.send_signal(signal.SIGTERM)
            server.wait(timeout=5)
