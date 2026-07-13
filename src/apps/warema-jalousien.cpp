/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "SemanticLog.h"
#include "express/legacy/in/WebApp.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>
#include <cstring>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

extern char** environ;

using namespace express;

static std::map<std::string, std::string> jalousien = {{"kueche", "kueche"},
                                                       {"strasse", "strasse"},
                                                       {"esstisch", "esstisch"},
                                                       {"balkon", "balkon"},
                                                       {"schlafzimmer", "schlafzimmer"},
                                                       {"arbeitszimmer", "arbeitszimmer"},
                                                       {"comfort", "komfort"},
                                                       {"all", "alle"}};

static std::map<std::string, std::string> actions = {{"open", "up"}, {"close", "down"}, {"stop", "stop"}};

int main(int argc, char* argv[]) {
    WebApp::init(argc, argv);

    legacy::in::WebApp webApp("werema");

    //    tls::WebApp wa;

    webApp.get("/jalousien/:id", [] APPLICATION(req, res) {
        snode::semantic::appLog().debug() << "Param: " << req->param("id");
        snode::semantic::appLog().debug() << "Query: " << req->query("action");

        const auto jalousieIt = jalousien.find(req->param("id"));
        const auto actionIt = actions.find(req->query("action"));
        if (jalousieIt == jalousien.end() || actionIt == actions.end()) {
            res->status(400).send("Unknown jalousie or action");
            return;
        }

        const std::string target = jalousieIt->second + "_" + actionIt->second;
        const std::string command = "aircontrol -t " + target;
        pid_t pid = 0;
        char program[] = "aircontrol";
        char option[] = "-t";
        std::vector<char> targetArg(target.begin(), target.end());
        targetArg.push_back('\0');
        char* const childArgv[] = {program, option, targetArg.data(), nullptr};

        // This example preserves the previous blocking request semantics, but avoids invoking a shell.
        const int spawnRet = posix_spawnp(&pid, program, nullptr, nullptr, childArgv, environ);
        if (spawnRet != 0) {
            if (spawnRet == ENOENT) {
                res->status(404).send("ERROR not found: " + command);
            } else {
                res->status(500).send("ERROR failed to start: " + command);
            }
            return;
        }

        int status = 0;
        pid_t waitedPid = 0;
        do {
            waitedPid = waitpid(pid, &status, 0);
        } while (waitedPid < 0 && errno == EINTR);

        if (waitedPid < 0) {
            res->status(500).send("ERROR wait failed: " + command);
        } else if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            res->status(200).send("OK: " + command);
        } else if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
            res->status(404).send("ERROR not found: " + command);
        } else {
            res->status(502).send("ERROR command failed: " + command);
        }
    });

    webApp.use([] APPLICATION(req, res) {
        res->status(404).send("No Jalousie specified");
    });

    webApp.listen(8080,
                  [instanceName = webApp.getConfig()->getInstanceName()](const legacy::in::WebApp::SocketAddress& socketAddress,
                                                                         const core::socket::State& state) {
                      switch (state) {
                          case core::socket::State::OK:
                              snode::semantic::appLog().info() << instanceName << ": listening on '" << socketAddress.toString() << "'";
                              break;
                          case core::socket::State::DISABLED:
                              snode::semantic::appLog().info() << instanceName << ": disabled";
                              break;
                          case core::socket::State::ERROR:
                              snode::semantic::appLog().error() << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                              break;
                          case core::socket::State::FATAL:
                              snode::semantic::appLog().critical()
                                  << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                              break;
                      }
                  });

    return WebApp::start();
}
