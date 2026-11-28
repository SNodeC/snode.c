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
from pathlib import Path
import subprocess
import sys
import tempfile
import time

from showcase_support import Server, ReplayProxy, records


def run():
    client, stdio, http, era, transport = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="mcp-tour-") as directory:
        server = None
        proxy = None
        command = [client, "--config-file", "/dev/null", "--quiet", "client", "--command", "tour", "--legacy", str(era == "2025-11-25").lower()]
        if transport == "stdio":
            command += ["--server", stdio, "--server-arguments", json.dumps(["--config-file", "/dev/null", "--quiet", "mcp", "--page-size", "3", "--deadline-ms", "2000", "--header-root", str(Path(__file__).resolve().parents[3] / "src")])]
        else:
            server = Server(http, directory, transport == "https", ["--demo-token", "tour-only"])
            if transport == "http" and era == "2025-11-25":
                proxy = ReplayProxy(server.port)
            command += ["--transport", transport, "--bearer-token", "tour-only", "mcp-client-" + transport, "remote", "--port", str(proxy.port if proxy else server.port)]
            if transport == "https":
                command += ["tls", "--ca-cert", str(server.cert)]
        try:
            wall = time.time()
            monotonic = time.monotonic()
            result = subprocess.run(command, capture_output=True, timeout=65)
            print(json.dumps({"wallElapsed": time.time() - wall, "monotonicElapsed": time.monotonic() - monotonic}), flush=True)
            output = result.stdout.decode(errors="replace")
            print(output, flush=True)
            print(result.stderr.decode(errors="replace"), file=sys.stderr, flush=True)
            assert result.returncode == 0, result.returncode
            values = records(output)
            assert values[-1]["tour"] == "PASS"
            steps = [entry for entry in values if "outcome" in entry]
            assert len(steps) == values[-1]["steps"] and len(steps) >= 50
            assert all(entry["outcome"] == "PASS" for entry in steps)
            starts = [entry for entry in values if "request" in entry]
            assert len(starts) == len(steps) and all(entry["request"] != "start" for entry in starts)
            assert {entry["feature"] for entry in steps} >= {
                "identity", "sampling.text", "sampling.tools", "sampling.choice.auto", "sampling.choice.none", "sampling.image", "sampling.audio", "sampling.context", "sampling.context.thisServer", "sampling.loop", "roots", "content", "schema.valid", "schema.invalid", "input.form", "input.url", "input.rounds", "input.parallel", "input.resource", "input.prompt", "input.task",
                "work.progress", "work.cancel", "work.deadline", "work.clientTimeout", "task.optional", "task.required",
                "task.get", "task.list", "task.result", "task.cancel", "dynamic.templates", "resource.update",
                "dynamic.resources.absent", "dynamic.templates.absent", "dynamic.prompts.absent",
                "completion.prompt", "completion.template", "notifications.summary"}
            features = {entry["feature"] for entry in steps}
            if era == "2025-11-25":
                assert features >= {"client.ping", "server.ping", "input.url.required",
                    "client.task.sampling.result", "client.task.sampling.cancel",
                    "client.task.elicitation.result", "client.task.elicitation.cancel"}
            else:
                assert features >= {"client.task.legacy-only", "client.ping.legacy-only"}
            assert "AddressSanitizer" not in result.stderr.decode(errors="replace")
            if proxy:
                assert proxy.cut and any(request["lastEventId"] for request in proxy.requests), proxy.requests
                print(json.dumps({"replay": "PASS", "requests": proxy.requests}), flush=True)
        finally:
            if proxy:
                proxy.stop()
            if server is not None:
                server.stop()


run()
