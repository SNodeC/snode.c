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

#include "Client.h"

#include "ClientConfig.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "mcp/client/Stdio.h"
#include "mcp/client/http/Client.h"
#include "net/in/stream/legacy/config/ConfigSocketClient.h"
#include "utils/Config.h"
#include "web/http/tls/in/Client.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <sys/wait.h> // IWYU pragma: keep - POSIX wait-status macros
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    int result = 1;
    bool success = false;
    const auto* config = utils::Config::configRoot.newSubCommand<apps::mcp::ClientConfig>();
    mcp::client::Stdio stdio("mcp-client-stdio", [&success, config] {
        return new apps::mcp::Client(success, *config);
    });
    mcp::client::http::Options options;
    options.bearerToken = [config] {
        return config->getBearerToken();
    };
    options.onError = [](const std::string& error) {
        std::cerr << error << std::endl;
        core::SNodeC::stop();
    };
    mcp::client::http::Http http("mcp-client-http", new apps::mcp::Client(success, *config), options);
    mcp::client::http::Https https("mcp-client-https", new apps::mcp::Client(success, *config), options);
    http.getConfig()->Remote::setHost("127.0.0.1")->setPort(8080);
    https.getConfig()->Remote::setHost("127.0.0.1")->setPort(8080);
    try {
        core::SNodeC::init(argc, argv);
        const std::string executable = config->getServer();
        if (!executable.empty()) {
            std::vector<std::string> arguments{executable};
            const auto extra = config->getServerArguments();
            arguments.insert(arguments.end(), extra.begin(), extra.end());
            stdio.start(std::move(arguments), [](int status) {
                if (status < 0) {
                    std::cerr << "Child spawn error: " << std::strerror(-status) << std::endl;
                } else if (WIFEXITED(status)) {
                    std::cerr << "Child exit code: " << WEXITSTATUS(status) << std::endl;
                } else if (WIFSIGNALED(status)) {
                    std::cerr << "Child signal: " << ::strsignal(WTERMSIG(status)) << std::endl;
                }
                core::SNodeC::stop();
            });
        } else if (config->getTransport() == "https") {
            https.setPath(config->getPath());
            https.start(https.getConfig()->Remote::getSocketAddress());
        } else {
            http.setPath(config->getPath());
            http.start(http.getConfig()->Remote::getSocketAddress());
        }
        core::timer::Timer watchdog;
        if (config->getCommand() == "tour") {
            watchdog = core::timer::Timer::singleshotTimer(
                [] {
                    std::cerr << "MCP client deadline expired" << std::endl;
                    core::SNodeC::stop();
                },
                utils::Timeval(60));
        }
        const int loop = core::SNodeC::start();
        result = success && loop == 0 ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        core::SNodeC::free();
    }
    return result;
}
