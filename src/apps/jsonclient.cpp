/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "core/SNodeC.h"
#include "web/http/legacy/in/Client.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using Client = web::http::legacy::in::Client;
    using Request = Client::Request;
    using Response = Client::Response;
    using SocketAddress = Client::SocketAddress;

    const Client jsonClient(
        "legacy",
        [](const std::shared_ptr<Request>& req) {
            VLOG(1) << "-- OnRequest";
            req->method = "POST";
            req->url = "/index.html";
            req->type("application/json");
            req->set("Connection", "close");
            req->send(
                "{\"userId\":1,\"schnitzel\":\"good\",\"hungry\":false}",
                [](const std::shared_ptr<Response>& res) {
                    VLOG(1) << "-- OnResponse";
                    VLOG(1) << "     Status:";
                    VLOG(1) << "       " << res->httpVersion;
                    VLOG(1) << "       " << res->statusCode;
                    VLOG(1) << "       " << res->reason;

                    VLOG(1) << "     Headers:";
                    for (const auto& [field, value] : res->headers) {
                        VLOG(1) << "       " << field + " = " + value;
                    }

                    VLOG(1) << "     Cookies:";
                    for (const auto& [name, cookie] : res->cookies) {
                        VLOG(1) << "       " + name + " = " + cookie.getValue();
                        for (const auto& [option, value] : cookie.getOptions()) {
                            VLOG(1) << "         " + option + " = " + value;
                        }
                    }

                    res->body.push_back(0);
                    VLOG(1) << "     Body:\n----------- start body -----------" << res->body.data() << "------------ end body ------------";
                },
                [](const std::string& message) {
                    VLOG(1) << "legacy: Request parse error: " << message;
                });
        },
        []([[maybe_unused]] const std::shared_ptr<Request>& req) {
            LOG(INFO) << " -- OnRequestEnd";
        });

    jsonClient.connect("localhost",
                       8080,
                       [instanceName = jsonClient.getConfig().getInstanceName()](
                           const SocketAddress& socketAddress,
                           const core::socket::State& state) { // example.com:81 simulate connect timeout
                           switch (state) {
                               case core::socket::State::OK:
                                   VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
                                   break;
                               case core::socket::State::DISABLED:
                                   VLOG(1) << instanceName << ": disabled";
                                   break;
                               case core::socket::State::ERROR:
                                   LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                   break;
                               case core::socket::State::FATAL:
                                   LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                   break;
                           }
                       });
    /*
        jsonClient.connect("localhost",
                           8080,
                           [instanceName = jsonClient.getConfig().getInstanceName()](
                               const SocketAddress& socketAddress,
                               const core::socket::State& state) { // example.com:81 simulate connnect timeout
                               switch (state) {
                                   case core::socket::State::OK:
                                       VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
                                       break;
                                   case core::socket::State::DISABLED:
                                       VLOG(1) << instanceName << ": disabled";
                                       break;
                                   case core::socket::State::ERROR:
                                       LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                       break;
                                   case core::socket::State::FATAL:
                                       LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                       break;
                               }
                           });
    */
    /*
        jsonClient.post("localhost", 8080, "/index.html", "{\"userId\":1,\"schnitzel\":\"good\",\"hungry\":false}", [](int err) {
            if (err != 0) {
                PLOG(ERROR) << "OnError: " << err;
            }
        });
    */

    return core::SNodeC::start();
}
