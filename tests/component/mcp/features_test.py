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

"""Public-wire coverage for server completion features in both protocol eras."""
from contextlib import contextmanager
import http.client
import json
import os
import sys
import tempfile

import http_test
import stdio_test


def message(method, params=None, modern=True, ident=1, capabilities=None):
    body = dict(params or {})
    if modern:
        body['_meta'] = dict(http_test.META)
        body['_meta']['io.modelcontextprotocol/clientCapabilities'] = capabilities or {}
    return {'jsonrpc': '2.0', 'id': ident, 'method': method, 'params': body}


def check_calls(call, modern):
    templates = call('resources/templates/list')['result']
    assert templates['resourceTemplates'][0]['uriTemplate'] == 'test://headers/{+path}'
    contents = call('resources/read', {'uri': 'test://headers/core%2FSocketContext.h'})['result']['contents']
    assert contents[0]['text'] == 'core/SocketContext.h'
    for reference in [{'type': 'ref/resource', 'uri': 'test://headers/{+path}'}, {'type': 'ref/prompt', 'name': 'complete'}]:
        result = call('completion/complete', {'ref': reference, 'argument': {'name': 'path' if reference['type'] == 'ref/resource' else 'value', 'value': 'Soc'},
                                            'context': {'arguments': {'other': 'value'}}})['result']
        assert result['completion']['values'] == ['Soc-completed']
    missing = call('resources/read', {'uri': 'missing://resource'})['error']['code']
    assert missing == (-32602 if modern else -32002)
    if modern:
        denied = call('tools/call', {'name': 'ask'})
        assert denied['error']['code'] == -32021
        response = call('tools/call', {'name': 'ask'}, {'elicitation': {'url': {}}})['result']
        assert response['resultType'] == 'input_required' and response['requestState'] == 'opaque'
        assert response['inputRequests']['choice']['params']['mode'] == 'url'
        retry = call('tools/call', {'name': 'ask', 'inputResponses': {'choice': {'action': 'accept'}}, 'requestState': 'echo'}, {'elicitation': {'url': {}}})
        assert retry['result']['content'][0]['text'] == 'echo'
        for method, parameters, key in (('resources/read', {'uri': 'test://ask'}, 'contents'), ('prompts/get', {'name': 'ask'}, 'messages')):
            pending = call(method, parameters, {'elicitation': {}})['result']
            assert pending['resultType'] == 'input_required'
            resumed = call(method, dict(parameters, inputResponses={'choice': {'action': 'accept'}}, requestState='echo'), {'elicitation': {}})['result']
            assert resumed[key]
        form = call('tools/call', {'name': 'ask', 'arguments': {'mode': 'form'}}, {'elicitation': {}})['result']
        assert form['inputRequests']['choice']['params']['mode'] == 'form'
        for action in ('accept', 'decline', 'cancel'):
            resumed = call('tools/call', {'name': 'ask', 'inputResponses': {'choice': {'action': action, 'content': {'label': 'safe'}}},
                                        'requestState': 'echo'}, {'elicitation': {}})
            assert resumed['result']['content'][0]['text'] == 'echo'
    else:
        assert call('tools/call', {'name': 'ask'})['result']['isError'] is True


@contextmanager
def stdio_client(executable):
    with tempfile.TemporaryDirectory() as directory:
        client = stdio_test.Client(executable, ["--config-file", "/dev/null", "--log-console", "stderr"], directory)
        try:
            yield client
        finally:
            client.finish()


def stdio(executable):
    for modern in (False, True):
        with stdio_client(executable) as client:
            counter = 10

            def call(method, params=None, capabilities=None):
                nonlocal counter
                counter += 1
                client.send(message(method, params, modern, counter, capabilities))
                response = client.receive()
                assert response['id'] == counter, response
                return response

            if not modern:
                assert call('initialize', {'protocolVersion': '2025-11-25', 'capabilities': {}, 'clientInfo': {'name': 'features', 'version': '1'}})['result']
            check_calls(call, modern)
            if modern:
                client.send(message('subscriptions/listen', {'notifications': {'resourceSubscriptions': ['test://updates']}}, True, 'watch'))
                ack = client.receive()
                assert ack['method'] == 'notifications/subscriptions/acknowledged'
                assert ack['params']['_meta']['io.modelcontextprotocol/subscriptionId'] == 'watch'
            else:
                call('resources/subscribe', {'uri': 'test://updates'})
            counter += 1
            client.send(message('tools/call', {'name': 'update'}, modern, counter))
            update = client.receive()
            assert update['method'] == 'notifications/resources/updated' and update['params']['uri'] == 'test://updates'
            assert client.receive()['id'] == counter
            if modern:
                client.send({'jsonrpc': '2.0', 'method': 'notifications/cancelled', 'params': {'requestId': 'watch'}})
            else:
                call('resources/unsubscribe', {'uri': 'test://updates'})
            call('tools/call', {'name': 'update'})
        print('PASS stdio', '2026-07-28' if modern else '2025-11-25')


def event(response):
    data = []
    while True:
        line = response.readline()
        assert line, 'notification stream closed unexpectedly'
        if line.startswith(b'data: '):
            data.append(line[6:].strip())
        if line in (b'\n', b'\r\n') and data:
            return json.loads(b'\n'.join(data))


def check_http(executable, curl):
    for modern in (False, True):
        with tempfile.TemporaryDirectory() as directory:
            server = http_test.Server(executable, directory, False, curl=curl)
            try:
                session = None
                counter = 10

                def fields(value):
                    result = http_test.headers(value)
                    if not modern:
                        result.update({'MCP-Protocol-Version': '2025-11-25', 'Mcp-Method': None, 'Mcp-Name': None})
                        if session:
                            result['Mcp-Session-Id'] = session
                    return result

                def call(method, params=None, capabilities=None):
                    nonlocal counter
                    counter += 1
                    value = message(method, params, modern, counter, capabilities)
                    status, head, body = server.curl(value, fields(value))
                    if b'text/event-stream' in head.lower():
                        response = json.loads(next(line[6:] for line in body.splitlines() if line.startswith(b'data: ')))
                    else:
                        response = json.loads(body)
                    assert response.get('id') == counter, (status, response)
                    return response

                if not modern:
                    initial = message('initialize', {'protocolVersion': '2025-11-25', 'capabilities': {}, 'clientInfo': {'name': 'features', 'version': '1'}}, False)
                    status, head, body = server.curl(initial, fields(initial))
                    assert status == 200 and 'result' in json.loads(body)
                    session = next(line.split(b': ', 1)[1].decode() for line in head.splitlines() if line.lower().startswith(b'mcp-session-id:'))
                check_calls(call, modern)
                connection = http.client.HTTPConnection('127.0.0.1', server.port, timeout=4)
                if modern:
                    value = message('subscriptions/listen', {'notifications': {'resourceSubscriptions': ['test://updates']}}, True, 'watch')
                    connection.request('POST', '/mcp', json.dumps(value), http_test.headers(value))
                else:
                    connection.request('GET', '/mcp', headers={'Accept': 'text/event-stream', 'Authorization': 'Bearer fixture',
                                                             'MCP-Protocol-Version': '2025-11-25', 'Mcp-Session-Id': session})
                stream = connection.getresponse()
                assert stream.status == 200
                if modern:
                    assert event(stream)['method'] == 'notifications/subscriptions/acknowledged'
                else:
                    call('resources/subscribe', {'uri': 'test://updates'})
                call('tools/call', {'name': 'update'})
                update = event(stream)
                assert update['method'] == 'notifications/resources/updated'
                assert update['params']['uri'] == 'test://updates'
                if modern:
                    assert update['params']['_meta']['io.modelcontextprotocol/subscriptionId'] == 'watch'
                connection.close()
                print('PASS HTTP', '2026-07-28' if modern else '2025-11-25')
            finally:
                server.stop()


if __name__ == '__main__':
    stdio(sys.argv[1])
    check_http(sys.argv[2], sys.argv[3])
