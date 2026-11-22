// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef CORE_SOCKET_STREAM_STDIO_SOCKETENDPOINT_HPP
#define CORE_SOCKET_STREAM_STDIO_SOCKETENDPOINT_HPP
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/socket/Socket.hpp"
#include "core/socket/stream/stdio/SocketConnection.hpp"
#include "core/socket/stream/stdio/SocketEndpoint.h"

#include <cerrno>
#include <exception>
#include <stdexcept>
#include <utility>

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
    const SocketEndpoint<PhysicalSocket, Config, Factory, Args...>&
    SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::setOnConnect(const std::function<void(SocketConnection*)>& callback,
                                                                           bool initialize) const {
        sharedContext->onConnect = initialize ? callback : [previous = sharedContext->onConnect, callback](SocketConnection* connection) {
            if (previous) {
                previous(connection);
            }
            if (callback) {
                callback(connection);
            }
        };
        return *this;
    }

    template <typename PhysicalSocket, typename Config, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    const SocketEndpoint<PhysicalSocket, Config, Factory, Args...>&
    SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::setOnConnected(const std::function<void(SocketConnection*)>& callback,
                                                                             bool initialize) const {
        sharedContext->onConnected =
            initialize ? callback : [previous = sharedContext->onConnected, callback](SocketConnection* connection) {
                if (previous) {
                    previous(connection);
                }
                if (callback) {
                    callback(connection);
                }
            };
        return *this;
    }

    template <typename PhysicalSocket, typename Config, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    const SocketEndpoint<PhysicalSocket, Config, Factory, Args...>&
    SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::setOnDisconnect(const std::function<void(SocketConnection*)>& callback,
                                                                              bool initialize) const {
        sharedContext->onDisconnect =
            initialize ? callback : [previous = sharedContext->onDisconnect, callback](SocketConnection* connection) {
                if (previous) {
                    previous(connection);
                }
                if (callback) {
                    callback(connection);
                }
            };
        return *this;
    }

    template <typename PhysicalSocket, typename Config, typename Factory, typename... Args>
        requires std::is_base_of_v<core::socket::stream::SocketContextFactory, Factory>
    std::shared_ptr<Factory> SocketEndpoint<PhysicalSocket, Config, Factory, Args...>::getSocketContextFactory() const {
        return sharedContext->factory;
    }
} // namespace core::socket::stream::stdio

#endif // CORE_SOCKET_STREAM_STDIO_SOCKETENDPOINT_HPP
