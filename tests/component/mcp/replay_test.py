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

import http.client
import json
import ssl
import sys
import tempfile
import time

import http_test


def run(executable, tls):
    for events, retention in [(128, 60000), (2, 60000), (128, 20)]:
        with tempfile.TemporaryDirectory(prefix='mcp-replay-') as directory:
            server = http_test.Server(executable, directory, tls,
                                     extra=['--fixture-replay-events', str(events), '--fixture-replay-ms', str(retention)], curl='curl')
            connections = []
            def connect(method, headers, value=None):
                connection = (http.client.HTTPSConnection('127.0.0.1', server.port, timeout=4, context=ssl._create_unverified_context())
                              if tls else http.client.HTTPConnection('127.0.0.1', server.port, timeout=4))
                connections.append(connection)
                connection.request(method, '/mcp', None if value is None else json.dumps(value), headers)
                return connection, connection.getresponse()
            def frame(response):
                ident, data = None, []
                complete = False
                while not complete:
                    line = response.readline()
                    assert line, 'unexpected SSE EOF'
                    if line.startswith(b'id:'):
                        ident = line[3:].strip().decode()
                    elif line.startswith(b'data:'):
                        data.append(line[5:].lstrip(b' ').rstrip(b'\r\n'))
                    elif line in (b'\n', b'\r\n'):
                        complete = ident is not None or bool(data)
                payload = b'\n'.join(data)
                return ident, json.loads(payload) if payload else None
            def initialize():
                value = {'jsonrpc':'2.0','id':'init','method':'initialize','params':{'protocolVersion':'2025-11-25',
                         'capabilities':{'elicitation':{}},'clientInfo':{'name':'replay-test','version':'1'}}}
                status, headers, body = server.curl(value, {'MCP-Protocol-Version':'2025-11-25','Mcp-Method':None})
                assert status == 200 and 'result' in json.loads(body)
                return next(line.split(b':',1)[1].strip().decode() for line in headers.splitlines() if line.lower().startswith(b'mcp-session-id:'))
            try:
                session = initialize()
                fields = {'Accept':'text/event-stream','Authorization':'Bearer fixture','MCP-Protocol-Version':'2025-11-25','Mcp-Session-Id':session}
                post = dict(fields, Accept='application/json, text/event-stream', **{'Content-Type':'application/json'})
                value = {'jsonrpc':'2.0','id':'subscribe','method':'resources/subscribe','params':{'uri':'test://updates'}}
                assert server.curl(value, post)[0] == 200
                stream, response = connect('GET', fields)
                assert response.status == 200
                initial, payload = frame(response)
                assert initial and payload is None
                response.close()
                stream.close()
                time.sleep(.03)
                for index in range(3):
                    value = {'jsonrpc':'2.0','id':index,'method':'tools/call','params':{'name':'update'}}
                    assert server.curl(value, post)[0] == 200
                if retention == 20:
                    time.sleep(1.1)
                resumed, response = connect('GET', dict(fields, **{'Last-Event-ID':initial}))
                if events == 2 or retention == 20:
                    assert response.status == 410, response.status
                else:
                    assert response.status == 200
                    seen = [frame(response) for _ in range(3)]
                    assert all(ident != initial and value['method'] == 'notifications/resources/updated' and
                               value['params']['uri'] == 'test://updates' for ident, value in seen)
                    assert len({ident for ident, _ in seen}) == 3
                    # A known event id cannot be used by another session.
                    _, denied = connect('GET', dict(fields, **{'Mcp-Session-Id':initialize(),'Last-Event-ID':initial}))
                    assert denied.status == 410
                    response.close()
                    resumed.close()
                    time.sleep(.03)
                    _, invalid = connect('GET', dict(fields, **{'Last-Event-ID':initial + 'wrong'}))
                    assert invalid.status == 410
                    # A detached POST response resumes its original execution, once.
                    post_session = initialize()
                    post_headers = dict(post, **{'Mcp-Session-Id':post_session})
                    delayed = {'jsonrpc':'2.0','id':'delayed','method':'tools/call','params':{'name':'delay','arguments':{'reportCount':True}}}
                    original, result = connect('POST', post_headers, delayed)
                    assert result.status == 200
                    event_id, empty = frame(result)
                    assert empty is None
                    result.close()
                    original.close()
                    time.sleep(.2)
                    _, completed = connect('GET', dict(fields, **{'Mcp-Session-Id':post_session,'Last-Event-ID':event_id}))
                    assert completed.status == 200
                    _, value = frame(completed)
                    assert value['id'] == 'delayed' and value['result']['structuredContent']['calls'] == 1
                    assert completed.read() == b''
                    _, second = connect('POST', post_headers, dict(delayed, id='second'))
                    frame(second)
                    _, value = frame(second)
                    assert value['result']['structuredContent']['calls'] == 2, 'resume must not rerun the handler'
                    assert second.read() == b''
                # DELETE must retire detached replay through the router's cleanup owner.
                deleted = initialize()
                deleted_headers = dict(fields, **{'Mcp-Session-Id':deleted})
                detached, seeded = connect('GET', deleted_headers)
                assert seeded.status == 200
                deleted_event, empty = frame(seeded)
                assert deleted_event and empty is None
                seeded.close()
                detached.close()
                time.sleep(.1)
                _, removed = connect('DELETE', deleted_headers)
                assert removed.status == 204 and removed.read() == b''
                # Cross the one-second cleanup interval before another request can clean it.
                time.sleep(1.2)
                assert server.process.poll() is None, 'server died after session deletion'
                _, retired = connect('GET', dict(deleted_headers, **{'Last-Event-ID':deleted_event}))
                assert retired.status == 404 and retired.read() == b''
                alive = initialize()
                value = {'jsonrpc':'2.0','id':'alive','method':'tools/list'}
                status, _, body = server.curl(value, dict(post, **{'Mcp-Session-Id':alive}))
                assert status == 200 and json.loads(body)['result']['tools']
                # Modern GET remains unsupported even with Last-Event-ID.
                _, modern = connect('GET', dict(fields, **{'MCP-Protocol-Version':'2026-07-28','Last-Event-ID':initial}))
                assert modern.status == 405
            finally:
                for connection in connections:
                    connection.close()
                server.stop()
    print('PASS: GET/POST replay, forced disconnect, exact stream/session binding, eviction, expiry, no duplicate execution, modern exclusion, DELETE cleanup and clean shutdown')


if __name__ == '__main__':
    run(sys.argv[1], sys.argv[2] == 'Tls')
