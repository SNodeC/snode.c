/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WEB_HTTP_CONFIGWEBSOCKET_H
#define WEB_HTTP_CONFIGWEBSOCKET_H

#include "utils/SubCommand.h"

#include <cstddef>
#include <string_view>

namespace web::http {

    class ConfigWebSocket : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"websocket"};
        constexpr static std::string_view DESCRIPTION{"WebSocket resource limits"};

        explicit ConfigWebSocket(utils::SubCommand* parent);
        ~ConfigWebSocket() override;

        ConfigWebSocket(const ConfigWebSocket&) = delete;
        ConfigWebSocket(ConfigWebSocket&&) noexcept = delete;
        ConfigWebSocket& operator=(const ConfigWebSocket&) = delete;
        ConfigWebSocket& operator=(ConfigWebSocket&&) noexcept = delete;

        ConfigWebSocket* setMaximumFrameBytes(std::size_t maximumFrameBytes);
        std::size_t getMaximumFrameBytes() const;

        ConfigWebSocket* setMaximumMessageBytes(std::size_t maximumMessageBytes);
        std::size_t getMaximumMessageBytes() const;

        ConfigWebSocket* setMaximumFragments(std::size_t maximumFragments);
        std::size_t getMaximumFragments() const;

    private:
        CLI::Option* maximumFrameBytesOpt = nullptr;
        CLI::Option* maximumMessageBytesOpt = nullptr;
        CLI::Option* maximumFragmentsOpt = nullptr;
    };

} // namespace web::http

#endif // WEB_HTTP_CONFIGWEBSOCKET_H
