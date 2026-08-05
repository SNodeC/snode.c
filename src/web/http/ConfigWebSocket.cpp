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

#include "web/http/ConfigWebSocket.h"

#include <string>

namespace web::http {

    ConfigWebSocket::ConfigWebSocket(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Resource Policies") {
        maximumFrameBytesOpt = addOption( //
            "--maximum-frame-bytes",
            "Maximum WebSocket frame payload size in bytes (0 = unlimited)",
            "bytes",
            std::size_t{0},
            CLI::NonNegativeNumber);
        maximumMessageBytesOpt = addOption( //
            "--maximum-message-bytes",
            "Maximum reassembled WebSocket message size in bytes (0 = unlimited)",
            "bytes",
            std::size_t{0},
            CLI::NonNegativeNumber);
        maximumFragmentsOpt = addOption( //
            "--maximum-fragments",
            "Maximum fragments per WebSocket message (0 = unlimited)",
            "count",
            std::size_t{0},
            CLI::NonNegativeNumber);
    }

    ConfigWebSocket::~ConfigWebSocket() {
    }

    ConfigWebSocket* ConfigWebSocket::setMaximumFrameBytes(std::size_t maximumFrameBytes) {
        setDefaultValue(maximumFrameBytesOpt, maximumFrameBytes);
        return this;
    }

    std::size_t ConfigWebSocket::getMaximumFrameBytes() const {
        return maximumFrameBytesOpt->as<std::size_t>();
    }

    ConfigWebSocket* ConfigWebSocket::setMaximumMessageBytes(std::size_t maximumMessageBytes) {
        setDefaultValue(maximumMessageBytesOpt, maximumMessageBytes);
        return this;
    }

    std::size_t ConfigWebSocket::getMaximumMessageBytes() const {
        return maximumMessageBytesOpt->as<std::size_t>();
    }

    ConfigWebSocket* ConfigWebSocket::setMaximumFragments(std::size_t maximumFragments) {
        setDefaultValue(maximumFragmentsOpt, maximumFragments);
        return this;
    }

    std::size_t ConfigWebSocket::getMaximumFragments() const {
        return maximumFragmentsOpt->as<std::size_t>();
    }

} // namespace web::http
