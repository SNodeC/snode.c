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

"""Legacy nested elicitation over real stdio and authenticated HTTP sessions."""
import http.client
import json
import sys
import tempfile

from features_test import event, message, stdio_client
import http_test


CAPABILITIES = {'elicitation': {'form': {}, 'url': {}}}
INITIAL = {'protocolVersion': '2025-11-25', 'capabilities': CAPABILITIES,
           'clientInfo': {'name': 'elicitation-test', 'version': '1'}}


def answer(request, action='accept'):
    result = {'action': action}
    if request['params']['mode'] == 'form' and action == 'accept':
        result['content'] = {'label': 'safe'}
    return {'jsonrpc': '2.0', 'id': request['id'], 'result': result}


def check_request(request, mode):
    assert request['method'] == 'elicitation/create', request
    assert request['params']['mode'] == mode
    if mode == 'url':
        assert request['params']['elicitationId'] and request['params']['url'].startswith('https://')
    else:
        assert request['params']['requestedSchema']['properties']['label']['type'] == 'string'


def stdio(executable):
    with stdio_client(executable) as client:
        client.send(message('initialize', INITIAL, False))
        assert client.receive()['result']['protocolVersion'] == '2025-11-25'
        identifier = 10
        for mode in ('form', 'url'):
            for action in ('accept', 'decline', 'cancel'):
                identifier += 1
                client.send(message('tools/call', {'name': 'ask', 'arguments': {'mode': mode}}, False, identifier))
                request = client.receive()
                check_request(request, mode)
                client.send(answer(request, action))
                result = client.receive()
                assert result['id'] == identifier and result['result']['content'][0]['text'] == 'opaque'
                assert 'resultType' not in result['result']
        for method, params, key in (('resources/read', {'uri': 'test://ask'}, 'contents'), ('prompts/get', {'name': 'ask'}, 'messages')):
            identifier += 1
            client.send(message(method, params, False, identifier))
            request = client.receive()
            check_request(request, 'form')
            client.send(answer(request))
            result = client.receive()
            assert result['id'] == identifier and result['result'][key]
        # Independent pending requests can finish in the opposite order.
        for identifier in (30, 31):
            client.send(message('tools/call', {'name': 'ask'}, False, identifier))
        requests = [client.receive(), client.receive()]
        client.send(answer(requests[1]))
        assert client.receive()['id'] == 31
        client.send(answer(requests[0]))
        assert client.receive()['id'] == 30
        client.send(message('tools/call', {'name': 'ask'}, False, 40))
        request = client.receive()
        client.send({'jsonrpc': '2.0', 'method': 'notifications/cancelled', 'params': {'requestId': 40}})
        assert client.receive()['params']['requestId'] == request['id']
        client.send(answer(request))  # Late replies must not resume or produce another response.
        client.send(message('ping', modern=False, ident=41))
        assert client.receive()['id'] == 41
        # EOF must release an outstanding nested request without waiting for its deadline.
        client.send(message('tools/call', {'name': 'ask'}, False, 42))
        assert client.receive()['method'] == 'elicitation/create'
    print('PASS legacy stdio: form/URL accept/decline/cancel, independent requests, cancellation, late reply, EOF')


def check_http(executable, curl):
    with tempfile.TemporaryDirectory() as directory:
        server = http_test.Server(executable, directory, False, curl=curl)
        try:
            fields = {'Authorization': 'Bearer fixture', 'Content-Type': 'application/json',
                      'Accept': 'application/json, text/event-stream', 'MCP-Protocol-Version': '2025-11-25',
                      'Mcp-Method': None, 'Mcp-Name': None}

            def initialize():
                status, head, body = server.curl(message('initialize', INITIAL, False), fields)
                assert status == 200 and 'result' in json.loads(body)
                return next(line.split(b': ', 1)[1].decode() for line in head.splitlines()
                            if line.lower().startswith(b'mcp-session-id:'))

            session = initialize()
            other = initialize()
            fields['Mcp-Session-Id'] = session
            identifier = 10

            def open_request(ident, method='tools/call', params=None):
                connection = http.client.HTTPConnection('127.0.0.1', server.port, timeout=4)
                value = message(method, params or {'name': 'ask', 'arguments': {'mode': mode}}, False, ident)
                connection.request('POST', '/mcp', json.dumps(value), {key: value for key, value in fields.items() if value is not None})
                stream = connection.getresponse()
                assert stream.status == 200 and stream.getheader('Content-Type') == 'text/event-stream'
                return connection, stream

            for mode in ('form', 'url'):
                for action in ('accept', 'decline', 'cancel'):
                    identifier += 1
                    connection, stream = open_request(identifier)
                    request = event(stream)
                    check_request(request, mode)
                    response = answer(request, action)
                    if identifier == 11:
                        for overrides in ({'Mcp-Session-Id': other}, {'Authorization': 'Bearer anonymous'}, {'Mcp-Session-Id': None}):
                            status, _, body = server.curl(response, dict(fields, **overrides))
                            assert status == 400 and json.loads(body)['error']['code'] == -32600, (status, body)
                        malformed = dict(response, result={'action': 'invalid'})
                        assert server.curl(malformed, fields)[0] == 400
                    status, _, body = server.curl(response, fields)
                    assert status == 202 and not body
                    result = event(stream)
                    assert result['id'] == identifier and result['result']['content'][0]['text'] == 'opaque'
                    assert 'resultType' not in result['result']
                    assert stream.read() == b''
                    connection.close()
                    assert server.curl(response, fields)[0] == 400
            for method, params, key in (('resources/read', {'uri': 'test://ask'}, 'contents'), ('prompts/get', {'name': 'ask'}, 'messages')):
                identifier += 1
                connection, stream = open_request(identifier, method, params)
                request = event(stream)
                check_request(request, 'form')
                assert server.curl(answer(request), fields)[0] == 202
                result = event(stream)
                assert result['id'] == identifier and result['result'][key]
                assert stream.read() == b''
                connection.close()
            connection, stream = open_request(40)
            request = event(stream)
            cancelled = {'jsonrpc': '2.0', 'method': 'notifications/cancelled', 'params': {'requestId': 40}}
            assert server.curl(cancelled, fields)[0] == 202
            assert event(stream)['params']['requestId'] == request['id']
            assert stream.read() == b''
            connection.close()
            assert server.curl(answer(request), fields)[0] == 400
            # Closing the originating stream releases correlations even while the session remains alive.
            connection, stream = open_request(41)
            request = event(stream)
            stream.close()
            connection.close()
            # Use an explicit session termination to guarantee cleanup without timing assumptions.
            assert server.curl(overrides=fields, method='DELETE')[0] == 204
            assert server.curl(answer(request), fields)[0] == 400
            print('PASS legacy HTTP: form/URL actions, original POST SSE, 202 replies, session/credential isolation, cancellation/cleanup')
        finally:
            server.stop()


if __name__ == '__main__':
    stdio(sys.argv[1])
    check_http(sys.argv[2], sys.argv[3])
