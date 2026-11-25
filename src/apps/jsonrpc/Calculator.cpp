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

#include "Calculator.h"

#include "core/SNodeC.h"
#include "jsonrpc/messages/Error.h"
#include "jsonrpc/messages/ErrorCode.h"
#include "jsonrpc/messages/Request.h"
#include "jsonrpc/messages/Result.h"
#include "log/SemanticLogger.h"
#include "utils/Timeval.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace apps::jsonrpc {

    Calculator::Calculator()
        : Super("calculator") {
    }

    Calculator::~Calculator() = default;

    void Calculator::onRequest(const ::jsonrpc::messages::Request& request) {
        if (request.method == "add") {
            if (request.params && request.params->is_array() && request.params->size() == 2 && (*request.params)[0].is_number() &&
                (*request.params)[1].is_number()) {
                sendResult(request.id, (*request.params)[0].get<double>() + (*request.params)[1].get<double>());
            } else {
                sendError(request.id, ::jsonrpc::messages::InvalidParams, "Expected two numbers");
            }
        } else if (request.method != "wait") {
            Super::onRequest(request);
        }
    }

    CalculatorClient::CalculatorClient()
        : Super("calculator-client") {
    }

    CalculatorClient::~CalculatorClient() = default;

    void CalculatorClient::onConnected() {
        Super::onConnected();
        sendRequest("add", nlohmann::json::array({20, 22}), utils::Timeval(1));
        sendRequest("unknown", {}, utils::Timeval(1));
        sendRequest("wait", {}, utils::Timeval(0.1));
    }

    void CalculatorClient::onResult(const ::jsonrpc::messages::Result& result) {
        log().info() << "calculator result: " << result.result;
    }

    void CalculatorClient::onError(const ::jsonrpc::messages::Error& error) {
        log().info() << "calculator error: " << error.code << " " << error.message;
    }

    void CalculatorClient::onRequestTimeout(const ::jsonrpc::messages::Id& id) {
        log().info() << "calculator timeout: " << id;
        core::SNodeC::stop();
    }

} // namespace apps::jsonrpc
