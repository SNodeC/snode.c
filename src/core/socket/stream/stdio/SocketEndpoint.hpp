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

#ifndef CORE_SOCKET_STREAM_STDIO_SOCKETENDPOINT_HPP
#define CORE_SOCKET_STREAM_STDIO_SOCKETENDPOINT_HPP

#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/socket/Socket.hpp"
#include "core/socket/stream/stdio/SocketConnection.hpp"
#include "core/socket/stream/stdio/SocketEndpoint.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <exception>
#include <stdexcept>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::stdio {

    template <typename PhysicalSocket, typename Config, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::SocketEndpoint(const std::string& name, Args&&... args)
        : Super(name)
        , sharedContext(std::make_shared<Context>(std::make_shared<Factory>(std::forward<Args>(args)...), nullptr, nullptr, nullptr, 0)) {
    }

    template <typename PhysicalSocket, typename Config, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    void SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::attach(const StatusCallback& onStatus) const {
        queue(std::nullopt, onStatus);
    }

    template <typename PhysicalSocket, typename Config, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    void SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::attach(const SocketAddress& address,
                                                                          const StatusCallback& onStatus) const {
        queue(address, onStatus);
    }

    template <typename PhysicalSocket, typename Config, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    void SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::queue(const std::optional<SocketAddress>& address,
                                                                         const StatusCallback& onStatus) const {
        core::EventReceiver::atNextTick([config = this->config, context = sharedContext, address, onStatus] {
            SocketAddress binding;
            if (address) {
                binding = *address;
            }

            if (core::SNodeC::state() != core::State::RUNNING) {
                onStatus(binding, core::socket::State(core::socket::State::ERROR, __FILE__, __LINE__, EINVAL, "Endpoint is not running"));
            } else if (config->Instance::getDisabled()) {
                onStatus(binding, core::socket::State(core::socket::State::DISABLED, __FILE__, __LINE__));
            } else {
                SocketConnection* connection = nullptr;
                auto established = std::make_shared<bool>(false);
                try {
                    if (!address) {
                        binding = config->Local::getSocketAddress();
                    }

                    binding.init();
                    auto* candidate = new SocketConnection(
                        PhysicalSocket(binding, config->Local::getDescriptorOwnership()),
                        [context, established](SocketConnection* ended) {
                            if (*established && context->onDisconnect) {
                                context->onDisconnect(ended);
                            }
                        },
                        ++context->connections,
                        config);
                    const bool active = candidate->activate([binding, onStatus](int error) {
                        onStatus(binding,
                                 core::socket::State(core::socket::State::ERROR, __FILE__, __LINE__, error, "Endpoint activation"));
                    });
                    if (active) {
                        connection = candidate;
                        *established = true;
                        if (context->onConnect) {
                            context->onConnect(connection);
                        }

                        if (core::SNodeC::state() == core::State::RUNNING && connection->isActive() && context->onConnected) {
                            context->onConnected(connection);
                        }

                        if (core::SNodeC::state() != core::State::RUNNING || !connection->isActive()) {
                            throw std::runtime_error("Endpoint closed during establishment");
                        }

                        auto* protocol = context->factory->create(connection);
                        if (protocol == nullptr) {
                            throw std::runtime_error("Endpoint factory returned no context");
                        }

                        connection->setSocketContext(protocol);
                    } else {
                        connection = nullptr;
                    }
                } catch (const std::exception& error) {
                    if (connection != nullptr) {
                        connection->close();
                    }

                    onStatus(binding, core::socket::State(core::socket::State::ERROR, __FILE__, __LINE__, EINVAL, error.what()));
                    connection = nullptr;
                } catch (...) {
                    if (connection != nullptr) {
                        connection->close();
                    }

                    onStatus(binding,
                             core::socket::State(core::socket::State::ERROR, __FILE__, __LINE__, EINVAL, "Endpoint callback failed"));
                    connection = nullptr;
                }
                if (connection != nullptr) {
                    onStatus(binding, core::socket::State(core::socket::State::OK, __FILE__, __LINE__));
                }
            }
        });
    }

    template <typename PhysicalSocket, typename Config, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    void SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::setCallback(std::function<void(SocketConnection*)>& current,
                                                                               const std::function<void(SocketConnection*)>& callback,
                                                                               bool initialize) {
        if (initialize) {
            current = callback;
        } else {
            current = [previous = current, callback](SocketConnection* connection) {
                if (previous) {
                    previous(connection);
                }

                if (callback) {
                    callback(connection);
                }
            };
        }
    }

    template <typename PhysicalSocket, typename Config, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    const SocketEndpoint<PhysicalSocket, Config, Factory, Args...>&
    SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::setOnConnect(const std::function<void(SocketConnection*)>& callback,
                                                                           bool initialize) const {
        setCallback(sharedContext->onConnect, callback, initialize);

        return *this;
    }

    template <typename PhysicalSocket, typename Config, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    const SocketEndpoint<PhysicalSocket, Config, Factory, Args...>&
    SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::setOnConnected(const std::function<void(SocketConnection*)>& callback,
                                                                             bool initialize) const {
        setCallback(sharedContext->onConnected, callback, initialize);

        return *this;
    }

    template <typename PhysicalSocket, typename Config, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    const SocketEndpoint<PhysicalSocket, Config, Factory, Args...>&
    SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::setOnDisconnect(const std::function<void(SocketConnection*)>& callback,
                                                                              bool initialize) const {
        setCallback(sharedContext->onDisconnect, callback, initialize);

        return *this;
    }

    template <typename PhysicalSocket, typename Config, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    std::shared_ptr<Factory> SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::getSocketContextFactory() const {
        return sharedContext->factory;
    }

} // namespace core::socket::stream::stdio

#endif // CORE_SOCKET_STREAM_STDIO_SOCKETENDPOINT_HPP
