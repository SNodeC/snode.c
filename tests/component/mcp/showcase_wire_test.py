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

"""Wire invariants not observable through the typed walkthrough's result hooks."""

import http.client
import json
from pathlib import Path
import ssl
import sys
import tempfile
import time

from showcase_support import Server
from stdio_test import Client

CAPABILITIES = {"sampling": {"tools": {}, "context": {}}, "roots": {"listChanged": True}, "elicitation": {"form": {}, "url": {}}, "extensions": {"io.modelcontextprotocol/tasks": {}}, "tasks": {"list": {}, "cancel": {}, "requests": {"sampling": {"createMessage": {}}, "elicitation": {"create": {}}}}}


def frame(response):
    event_id = None
    data = []
    complete = False
    while not complete:
        line = response.readline()
        assert line, "unexpected SSE EOF"
        if line.startswith(b"id:"):
            event_id = line[3:].strip().decode()
        elif line.startswith(b"data:"):
            data.append(line[5:].lstrip(b" ").rstrip(b"\r\n"))
        elif line in (b"\n", b"\r\n"):
            complete = event_id is not None or bool(data)
    payload = b"\n".join(data)
    return event_id, json.loads(payload) if payload else None


class Wire:
    def __init__(self, stdio, http, era, transport, directory):
        self.modern = era == "2026-07-28"
        self.era = era
        self.ident = 0
        self.session = None
        self.server = None
        self.connections = []
        self.notifications = []
        self.client = None
        if transport == "stdio":
            self.client = Client(stdio, ["--config-file", "/dev/null", "--quiet", "mcp", "--page-size", "3", "--deadline-ms", "2000", "--header-root", str(Path(__file__).resolve().parents[3] / "src")], directory)
        else:
            self.server = Server(http, directory, transport == "https", ["--demo-token", "wire-only", "--authorization-server", "https://issuer.example/",
                                                                     "--resource-uri", "https://resource.example/mcp"])
        initial = self.call("server/discover" if self.modern else "initialize",
                            {} if self.modern else {"protocolVersion": era, "clientInfo": {"name": "showcase-wire", "version": "1"}, "capabilities": CAPABILITIES})["result"]
        info = initial.get("serverInfo", initial.get("_meta", {}).get("io.modelcontextprotocol/serverInfo"))
        assert info["name"] == "snodec-mcp-showcase" and info["title"] and info["icons"] and info["websiteUrl"]
        assert initial["capabilities"]["resources"]["subscribe"] is True
        assert "logging" in initial["capabilities"]
        assert initial["instructions"]
        if not self.modern:
            self.notify("notifications/initialized", {})

    def message(self, method, params=None):
        self.ident += 1
        params = dict(params or {})
        if self.modern:
            capabilities = dict(CAPABILITIES, roots={})
            capabilities.pop("tasks")
            params["_meta"] = dict(params.get("_meta", {}), **{"io.modelcontextprotocol/protocolVersion": self.era,
                                "io.modelcontextprotocol/clientCapabilities": capabilities})
        return {"jsonrpc": "2.0", "id": self.ident, "method": method, "params": params}

    def headers(self, value=None):
        fields = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream", "Authorization": "Bearer wire-only",
                  "MCP-Protocol-Version": self.era}
        if self.session:
            fields["Mcp-Session-Id"] = self.session
        if self.modern and value and "method" in value:
            method = value["method"]
            params = value.get("params", {})
            fields["Mcp-Method"] = method
            if method in ("tools/call", "prompts/get", "resources/read"):
                fields["Mcp-Name"] = params.get("name", params.get("uri", ""))
            if method == "tools/call" and params.get("name") == "headers":
                fields["Mcp-Param-Region"] = params["arguments"]["region"]
        return fields

    def http(self, method="POST", value=None, overrides=None, path="/mcp"):
        fields = self.headers(value)
        fields.update(overrides or {})
        fields = {key: value for key, value in fields.items() if value is not None}
        connection = http.client.HTTPSConnection("127.0.0.1", self.server.port, timeout=5, context=ssl.create_default_context(cafile=str(self.server.cert))) if self.server.tls else http.client.HTTPConnection("127.0.0.1", self.server.port, timeout=5)
        self.connections.append(connection)
        connection.request(method, path, None if value is None else json.dumps(value), fields)
        response = connection.getresponse()
        if response.getheader("Mcp-Session-Id"):
            self.session = response.getheader("Mcp-Session-Id")
        return connection, response

    def notify(self, method, params):
        value = self.message(method, params)
        del value["id"]
        if self.client:
            self.client.send(value)
        else:
            connection, response = self.http(value=value)
            assert response.status == 202, (response.status, response.read())
            response.read()
            connection.close()

    def call(self, method, params=None, overrides=None):
        value = self.message(method, params)
        if self.client:
            self.client.send(value)
            result = self.client.receive()
            while result.get("id") != value["id"]:
                self.notifications.append(result)
                result = self.client.receive()
        else:
            connection, response = self.http(value=value, overrides=overrides)
            assert response.status in (200, 400), (response.status, response.read())
            if "text/event-stream" in response.getheader("Content-Type", ""):
                result = None
                while result is None or result.get("id") != value["id"]:
                    _, result = frame(response)
                    if result is not None and result.get("id") != value["id"]:
                        self.notifications.append(result)
            else:
                result = json.loads(response.read())
            connection.close()
        print(json.dumps({"request": value, "response": result}, ensure_ascii=False, indent=4), flush=True)
        return result

    def client_input(self, name, arguments, result, expected=None):
        value = self.message("tools/call", {"name": name, "arguments": arguments})
        expected_method = "roots/list" if name == "roots" else "elicitation/create" if name == "input.url" else "sampling/createMessage"
        seen = []
        def answer(request):
            seen.append(request)
            assert request["method"] == expected_method
            token = request.get("params", {}).get("_meta", {}).get("progressToken")
            if token is not None:
                for progress, total in ((1, 2), (1, 2), (0, 2), (2, 1), (2, 2)):
                    self.notify("notifications/progress", {"progressToken": token, "progress": progress, "total": total})
            response = result(request) if callable(result) else result
            return response
        if self.modern:
            response = self.call("tools/call", {"name": name, "arguments": arguments})["result"]
            while response.get("resultType") == "input_required":
                responses = {key: answer(request) for key, request in response["inputRequests"].items()}
                response = self.call("tools/call", {"name": name, "arguments": arguments,
                    "requestState": response["requestState"], "inputResponses": responses})["result"]
        else:
            connection = None
            stream = None
            if self.client:
                self.client.send(value)
            else:
                connection, stream = self.http(value=value)
                assert stream.status == 200 and "text/event-stream" in stream.getheader("Content-Type", "")
            complete = False
            while not complete:
                message = self.client.receive() if self.client else frame(stream)[1]
                if message is not None and message.get("method") == expected_method:
                    reply = {"jsonrpc": "2.0", "id": message["id"], "result": answer(message)}
                    if self.client:
                        self.client.send(reply)
                    else:
                        reply_connection, received = self.http(value=reply)
                        assert received.status == 202 and received.read() == b""
                        reply_connection.close()
                elif message is not None and message.get("id") == value["id"]:
                    response = message["result"]
                    complete = True
                elif message is not None:
                    self.notifications.append(message)
            if connection:
                connection.close()
        if any("progressToken" in request.get("params", {}).get("_meta", {}) for request in seen):
            assert response["_meta"]["showcase/clientProgress"] == 2, response
        actual = dict(response["structuredContent"])
        changes = actual.pop("listChanges") if name == "roots" else None
        assert actual == (result if expected is None else expected), response
        if name == "roots":
            assert changes == getattr(self, "roots_changes", 0), response
        print(json.dumps({"clientInput": expected_method, "requests": seen, "result": actual}, indent=4), flush=True)
        return seen

    def client_task(self, kind, cancel):
        value = self.message("tools/call", {"name": "client.task", "arguments": {"kind": kind, "cancel": cancel}})
        connection = None
        stream = None
        if self.client:
            self.client.send(value)
        else:
            connection, stream = self.http(value=value)
            assert stream.status == 200 and "text/event-stream" in stream.getheader("Content-Type", "")
        task = {"taskId": "wire-client-task", "status": "working", "createdAt": "2026-01-01T00:00:00Z",
                "lastUpdatedAt": "2026-01-01T00:00:00Z", "ttl": 60000, "pollInterval": 20}
        payload = {"role": "assistant", "model": "wire-model", "content": {"type": "text", "text": "42"}} if kind == "sampling" else {"action": "decline"}
        payload["_meta"] = {"io.modelcontextprotocol/related-task": {"taskId": task["taskId"]}}
        methods = []
        complete = False
        while not complete:
            message = self.client.receive() if self.client else frame(stream)[1]
            if message is not None and "method" in message and "id" in message:
                method = message["method"]
                methods.append(method)
                params = message.get("params", {})
                if method in ("sampling/createMessage", "elicitation/create"):
                    assert params["task"]["ttl"] == 60000
                    self.notify("notifications/progress", {"progressToken": params["_meta"]["progressToken"], "progress": 1, "total": 1,
                                "_meta": {"io.modelcontextprotocol/related-task": {"taskId": task["taskId"]}}})
                    response = {"task": task, "_meta": {"io.modelcontextprotocol/related-task": {"taskId": task["taskId"]}}}
                elif method == "tasks/get":
                    assert params["taskId"] == task["taskId"]
                    response = task
                elif method == "tasks/list":
                    response = {"tasks": [task]}
                elif method == "tasks/result":
                    assert not cancel and params["taskId"] == task["taskId"]
                    response = payload
                elif method == "tasks/cancel":
                    assert cancel and params["taskId"] == task["taskId"]
                    response = dict(task, status="cancelled")
                else:
                    raise AssertionError(message)
                reply = {"jsonrpc": "2.0", "id": message["id"], "result": response}
                if self.client:
                    self.client.send(reply)
                else:
                    reply_connection, received = self.http(value=reply)
                    assert received.status == 202 and received.read() == b""
                    reply_connection.close()
            elif message is not None and message.get("id") == value["id"]:
                assert message["result"]["_meta"]["showcase/clientProgress"] == 1, message
                actual = message["result"]["structuredContent"]
                assert actual["status"] == ("cancelled" if cancel else "completed"), message
                assert actual["payload"] == (dict(task, status="cancelled") if cancel else payload), message
                complete = True
            elif message is not None:
                self.notifications.append(message)
        assert methods == ["sampling/createMessage" if kind == "sampling" else "elicitation/create", "tasks/list", "tasks/get", "tasks/cancel" if cancel else "tasks/result"], methods
        if connection:
            connection.close()
        print(json.dumps({"clientTask": kind, "cancel": cancel, "methods": methods}, indent=4), flush=True)

    def close(self):
        for connection in self.connections:
            connection.close()
        if self.client:
            self.client.finish()
        if self.server:
            self.server.stop()


def check(wire):
    tools = []
    cursor = None
    pages = 0
    while cursor is not None or pages == 0:
        page = wire.call("tools/list", {"cursor": cursor} if cursor is not None else {})["result"]
        assert len(page["tools"]) <= 3
        tools.extend(page["tools"])
        cursor = page.get("nextCursor")
        pages += 1
    assert pages > 3 and len({tool["name"] for tool in tools}) == len(tools)
    described = next(tool for tool in tools if tool["name"] == "text")
    assert described["title"] == "text" and described["icons"] and described["_meta"]["showcase/feature"] == "text"
    assert described["annotations"] == {"readOnlyHint": True, "destructiveHint": False, "idempotentHint": True, "openWorldHint": False}
    result = wire.call("tools/call", {"name": "text", "arguments": {}})["result"]
    assert result["_meta"]["showcase/result"] == "text", result
    assert result["content"] == [{"type": "text", "text": "Hello from the MCP showcase."}], result
    for args in [{"word": "123", "choice": 1}, {"word": "Ada", "choice": 3}, {"word": "Ada", "choice": 1, "extra": True}]:
        assert wire.call("tools/call", {"name": "schema", "arguments": args})["result"]["isError"] is True
    for uri, content, ttl, scope in [("showcase://text", "text", 1000, "public"), ("showcase://blob", "blob", 500, "private")]:
        result = wire.call("resources/read", {"uri": uri})["result"]
        assert content in result["contents"][0]
        if wire.modern:
            assert result["resultType"] == "complete" and result["ttlMs"] == ttl and result["cacheScope"] == scope
        else:
            assert not {"resultType", "ttlMs", "cacheScope"} & result.keys()
    missing = wire.call("resources/read", {"uri": "showcase://missing"})["error"]["code"]
    assert missing == (-32602 if wire.modern else -32002)
    if wire.modern:
        for mode in ("form", "url", "rounds"):
            result = wire.call("tools/call", {"name": "input." + mode})["result"]
            assert result["resultType"] == "input_required" and result["requestState"] == "first"
            assert result["inputRequests"]["first"]["method"] == "elicitation/create"
            assert result["inputRequests"]["first"]["params"]["mode"] == ("url" if mode == "url" else "form")
            for action in ("accept", "decline", "cancel"):
                answer = {"action": action}
                if action == "accept" and mode != "url":
                    answer["content"] = {"label": "wire-only"}
                retry = wire.call("tools/call", {"name": "input." + mode, "requestState": "first", "inputResponses": {"first": answer}})["result"]
                if mode == "rounds" and action == "accept":
                    assert retry["resultType"] == "input_required" and retry["requestState"] == "second"
                else:
                    assert retry["structuredContent"]["action"] == action
    wire.client_input("sampling", {}, {"role": "assistant", "model": "wire-model", "content": {"type": "text", "text": "42"}, "_meta": {"wire/sample": True}})
    wire.client_input("sampling", {"toolUse": True}, {"role": "assistant", "model": "wire-model", "content": [{"type": "tool_use", "id": "wire-tool", "name": "answer", "input": {}}], "stopReason": "toolUse"})
    final = {"role": "assistant", "model": "wire-model", "content": {"type": "text", "text": "42"}}
    for choice in ("auto", "none", "required"):
        chosen = final if choice != "required" else {"role": "assistant", "model": "wire-model", "content": [{"type": "tool_use", "id": "choice", "name": "answer", "input": {}}]}
        def choice_answer(request):
            assert request["params"]["toolChoice"] == {"mode": choice}
            return chosen
        wire.client_input("sampling", {"toolChoice": choice}, choice_answer, chosen)
    for context in ("none", "thisServer", "allServers"):
        def context_answer(request):
            assert request["params"]["includeContext"] == context
            return final
        wire.client_input("sampling", {"context": context}, context_answer, final)
    def loop_answer(request):
        params = request["params"]
        if params["toolChoice"]["mode"] == "required":
            result = {"role": "assistant", "model": "wire-model", "content": [{"type": "tool_use", "id": "loop", "name": "answer", "input": {}}], "stopReason": "toolUse"}
        else:
            assert params["toolChoice"]["mode"] == "none"
            assert params["messages"][-1]["content"][0]["toolUseId"] == "loop"
            assert params["messages"][-1]["content"][0]["structuredContent"] == {"answer": 42}
            result = final
        return result
    loop = wire.client_input("sampling", {"toolLoop": True}, loop_answer, final)
    assert len(loop) == 2
    for media in ("image", "audio"):
        def media_answer(request):
            params = request["params"]
            assert params["messages"][0]["content"]["type"] == media
            assert params["systemPrompt"] and params["temperature"] == 0.25
            assert params["modelPreferences"]["hints"] and params["metadata"] == {"showcase": True}
            assert params["includeContext"] == "allServers" and params["stopSequences"] == ["END"]
            return final
        wire.client_input("sampling", {"media": media, "options": True, "context": "allServers"}, media_answer, final)
    url = wire.client_input("input.url", {}, {"action": "accept"}, {"action": "accept", "state": "first", "completionNotified": not wire.modern})
    if not wire.modern:
        assert any(event.get("method") == "notifications/elicitation/complete" and event["params"]["elicitationId"] == url[0]["params"]["elicitationId"] for event in wire.notifications)
        required = wire.call("tools/call", {"name": "input.url", "arguments": {"required": True}})
        assert required["error"]["code"] == -32042 and required["error"]["data"]["elicitations"][0]["mode"] == "url"
        assert wire.call("ping")["result"] == {}
        for kind in ("sampling", "elicitation"):
            for cancel in (False, True):
                wire.client_task(kind, cancel)
    if wire.modern:
        before = len(wire.notifications)
        no_logs = wire.call("tools/call", {"name": "logging"})
        assert no_logs["result"]["structuredContent"]["requestLogAdmitted"] is False
        assert len(wire.notifications) == before
        logged = wire.call("tools/call", {"name": "logging", "_meta": {"io.modelcontextprotocol/logLevel": "debug"}})
        assert logged["result"]["structuredContent"]["requestLogAdmitted"] is True
        assert len(wire.notifications) == before + 1 and wire.notifications[-1]["method"] == "notifications/message"
        assert wire.call("tools/call", {"name": "logging", "_meta": {"io.modelcontextprotocol/logLevel": "trace"}})["error"]["code"] == -32602
    wire.client_input("roots", {}, {"roots": [{"uri": "file:///wire/workspace", "name": "Wire workspace"}], "_meta": {"wire/roots": True}})
    if not wire.modern:
        wire.notify("notifications/roots/list_changed", {})
        wire.roots_changes = 1
        wire.client_input("roots", {}, {"roots": [{"uri": "file:///wire/changed", "name": "Changed workspace"}]})
    if wire.server:
        auth(wire)
        if not wire.modern:
            replay(wire)


def auth(wire):
    for credential, status, error in [(None, 401, None), ("Bearer wire-only:read", 403, "insufficient_scope"), ("Bearer wire-only:forbidden", 403, None)]:
        _, response = wire.http(value=wire.message("tools/list"), overrides={"Authorization": credential})
        assert response.status == status, (response.status, response.read())
        challenge = response.getheader("WWW-Authenticate", "")
        print(json.dumps({"authorization": {"httpStatus": status, "wwwAuthenticate": challenge}}, indent=4), flush=True)
        if status == 401 or error:
            assert "resource_metadata=" in challenge and "Bearer" in challenge
        if error:
            assert 'error="insufficient_scope"' in challenge and 'scope="showcase:write"' in challenge
        response.read()
    for path in ("/.well-known/oauth-protected-resource", "/.well-known/oauth-protected-resource/mcp"):
        _, response = wire.http(method="GET", path=path, overrides={"Authorization": None})
        assert response.status == 200
        metadata = json.loads(response.read())
        assert metadata["resource"] == "https://resource.example/mcp" and metadata["authorization_servers"] == ["https://issuer.example/"]
    _, response = wire.http(value=wire.message("tools/list"), overrides={"Origin": "https://untrusted.example"})
    assert response.status == 403
    response.read()
    if wire.modern:
        mismatch = wire.call("tools/call", {"name": "headers", "arguments": {"region": "eu"}}, {"Mcp-Param-Region": "us"})
        assert mismatch["error"]["code"] == -32020


def replay(wire):
    assert wire.call("resources/subscribe", {"uri": "showcase://counter"})["result"] == {}
    original, response = wire.http(method="GET", overrides={"Accept": "text/event-stream"})
    assert response.status == 200
    event_id, empty = frame(response)
    assert event_id and empty is None
    response.close()
    original.close()
    time.sleep(0.03)
    wire.call("tools/call", {"name": "update"})
    resumed, response = wire.http(method="GET", overrides={"Accept": "text/event-stream", "Last-Event-ID": event_id})
    assert response.status == 200
    next_id, notification = frame(response)
    assert next_id != event_id and notification["method"] == "notifications/resources/updated"
    assert notification["params"]["uri"] == "showcase://counter"
    response.close()
    resumed.close()
    time.sleep(0.03)
    post, response = wire.http(value=wire.message("tools/call", {"name": "work", "arguments": {"steps": 3, "delayMs": 50}}))
    assert response.status == 200
    event_id, empty = frame(response)
    assert event_id and empty is None
    response.close()
    post.close()
    time.sleep(0.2)
    resumed, response = wire.http(method="GET", overrides={"Accept": "text/event-stream", "Last-Event-ID": event_id})
    assert response.status == 200
    _, result = frame(response)
    assert result["result"]["structuredContent"]["completed"] == 3
    assert response.read() == b""
    resumed.close()
    print("PASS: forced GET/POST disconnect and Last-Event-ID replay", flush=True)


def run():
    stdio, http, era, transport = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="mcp-showcase-wire-") as directory:
        wire = Wire(stdio, http, era, transport, directory)
        try:
            check(wire)
        finally:
            wire.close()
    print("PASS: wire metadata, paging, schema enforcement, caching, input, auth and replay", flush=True)


run()
