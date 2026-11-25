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

"""A disconnected stdio owner releases completed and working task retention."""
import json
import os
import select
import subprocess
import sys
import time

for legacy in (True, False):
    for working in (False, True):
        peer = subprocess.Popen([sys.argv[1], '--config-file', '/dev/null', '--log-console', 'stderr'],
                                stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        pending = bytearray()
        def send(value):
            peer.stdin.write((json.dumps(value)+'\n').encode())
            peer.stdin.flush()
        def receive(ident):
            result = None
            deadline = time.monotonic()+3
            while result is None and time.monotonic()<deadline:
                if b'\n' not in pending:
                    ready, _, _ = select.select([peer.stdout], [], [], max(0, deadline-time.monotonic()))
                    assert ready, 'response timeout'
                    data = os.read(peer.stdout.fileno(), 65536)
                    assert data, 'unexpected EOF'
                    pending.extend(data)
                while b'\n' in pending and result is None:
                    line, _, rest = pending.partition(b'\n')
                    pending[:] = rest
                    message = json.loads(line)
                    if message.get('id') == ident:
                        result = message
            assert result is not None
            return result
        try:
            caps = {'extensions': {'io.modelcontextprotocol/tasks': {}}}
            if legacy:
                send({'jsonrpc':'2.0','id':1,'method':'initialize','params':{'protocolVersion':'2025-11-25',
                      'capabilities':caps,'clientInfo':{'name':'linger','version':'1'}}})
                assert 'result' in receive(1)
                send({'jsonrpc':'2.0','method':'notifications/initialized'})
            params = {'name':'background','arguments':{'ttl':20000,'waitForCancel':working}}
            if legacy:
                params['task'] = {'ttl':20000}
            else:
                params['_meta'] = {'io.modelcontextprotocol/protocolVersion':'2026-07-28',
                                   'io.modelcontextprotocol/clientCapabilities':caps}
            send({'jsonrpc':'2.0','id':2,'method':'tools/call','params':params})
            handle = receive(2)['result']
            handle = handle['task'] if legacy else handle
            assert handle['ttl' if legacy else 'ttlMs'] == 20000
            query = {'taskId':handle['taskId']}
            if not legacy:
                query['_meta'] = params['_meta']
            status = 'working'
            deadline = time.monotonic()+3
            while status == 'working' and time.monotonic()<deadline:
                send({'jsonrpc':'2.0','id':3,'method':'tasks/get','params':query})
                status = receive(3)['result']['status']
                if working:
                    assert status == 'working', status
                    deadline = 0
                elif status == 'working':
                    time.sleep(.01)
            assert status == ('working' if working else 'completed'), status
            started = time.monotonic()
            peer.stdin.close()
            peer.wait(timeout=1)
            assert peer.returncode == 0, peer.returncode
            assert time.monotonic()-started < 1
            print('legacy' if legacy else 'modern', 'working' if working else 'completed', 'PASS')
        finally:
            if peer.poll() is None:
                peer.kill()
                peer.wait()
