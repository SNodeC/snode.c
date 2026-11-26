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

"""Protect the development assistant inventories from showcase registration."""

import http.client
import json
import subprocess
import sys
import tempfile

from http_test import Server
from stdio_test import Client, request

# Registrations in 69b16419a:src/apps/mcp/main.cpp and their concrete definitions.
EXPECTED = {
    "tools/list": ("tools", "name", {"api.find", "api.read", "skeleton.preview", "review.label"}),
    "resources/list": ("resources", "uri", {"snodec://development/house-style", "snodec://development/review-label"}),
    "prompts/list": ("prompts", "name", {"snodec-review"}),
    "resources/templates/list": ("resourceTemplates", "uriTemplate", {"snodec://development/style/{section}"}),
}


def run():
    executable, transport, era = sys.argv[1:]
    modern = era == "2026-07-28"
    with tempfile.TemporaryDirectory(prefix="mcp-dev-inventory-") as directory:
        args = ["--config-file", "/dev/null", "--quiet"]
        client = Client(executable, args, directory) if transport == "stdio" else None
        server = Server(executable, directory, False, curl="curl") if transport == "http" else None
        session = None
        ident = 0

        def exchange(method, params=None, notification=False):
            nonlocal session, ident
            ident += 1
            value = request(method, ident, params, modern)
            if notification:
                del value["id"]
            result = None
            if client:
                client.send(value)
                if not notification:
                    result = client.receive()
            else:
                connection = http.client.HTTPConnection("127.0.0.1", server.port, timeout=4)
                fields = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream", "MCP-Protocol-Version": era}
                if modern:
                    fields["Mcp-Method"] = method
                if session:
                    fields["Mcp-Session-Id"] = session
                connection.request("POST", "/mcp", json.dumps(value), fields)
                response = connection.getresponse()
                assert response.status == (202 if notification else 200), response.read()
                session = response.getheader("Mcp-Session-Id") or session
                body = response.read()
                if not notification:
                    result = json.loads(body)
                connection.close()
            return result

        try:
            identity = exchange("server/discover" if modern else "initialize", {} if modern else {
                "protocolVersion": era, "capabilities": {}, "clientInfo": {"name": "dev-inventory-test", "version": "1"}})["result"]
            info = identity.get("serverInfo", identity.get("_meta", {}).get("io.modelcontextprotocol/serverInfo"))
            assert info["name"] == "snodec-dev"
            if not modern:
                exchange("notifications/initialized", notification=True)
            for method, (field, key, expected) in EXPECTED.items():
                result = exchange(method)["result"]
                assert not result.get("nextCursor"), (method, "original inventory must fit one page", result)
                entries = result[field]
                assert len(entries) == len(expected) and {e[key] for e in entries} == expected, (method, entries)
                wire = json.dumps(entries)
                assert "showcase://" not in wire and "catalogue." not in wire
                print(json.dumps({"era": era, "transport": transport, "method": method, "pages": 1, "entries": sorted(expected)}), flush=True)
            if server:
                for path in ["/showcase/approve", "/.well-known/oauth-protected-resource/mcp"]:
                    connection = http.client.HTTPConnection("127.0.0.1", server.port, timeout=4)
                    connection.request("GET", path)
                    response = connection.getresponse()
                    assert response.status == 404, (path, response.status)
                    response.read()
                    connection.close()
        finally:
            if client:
                client.finish()
            if server:
                server.stop()
        section = [] if transport == "stdio" else ["mcp-http"]
        help_result = subprocess.run([executable] + args + section + ["mcp", "--help"], capture_output=True, timeout=5)
        assert help_result.returncode == 2, (help_result.returncode, help_result.stderr)
        help_text = help_result.stdout + help_result.stderr
        assert b"--page-size" in help_text and b"[128]" in help_text, help_text
        assert all(option not in help_text for option in [b"--demo-token", b"--authorization-server", b"--resource-uri", b"--replay-events"])


if __name__ == "__main__":
    run()
