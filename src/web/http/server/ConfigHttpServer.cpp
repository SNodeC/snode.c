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

#include "web/http/server/ConfigHttpServer.h"

#include "web/http/ConfigHttpParser.h"

#include <string>

namespace web::http::server {

    ConfigHttpServer::ConfigHttpServer(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Applications") {
        maximumPendingRequestsOpt = addOption( //
            "--maximum-pending-requests",
            "Maximum pending HTTP requests per connection (0 = unlimited)",
            "count",
            std::size_t{0},
            CLI::NonNegativeNumber);
        allowChunkedTransferOpt = addFlag( //
            "--allow-chunked-transfer{true}",
            "Allow chunked request transfer encoding",
            "BOOL",
            "true",
            CLI::IsMember({"true", "false"}));
        allowPipeliningOpt = addFlag( //
            "--allow-pipelining{true}",
            "Allow pipelined HTTP requests",
            "BOOL",
            "true",
            CLI::IsMember({"true", "false"}));

        newSubCommand<web::http::ConfigHttpParser>();
    }

    ConfigHttpServer::~ConfigHttpServer() {
    }

    ConfigHttpServer* ConfigHttpServer::setMaximumPendingRequests(std::size_t maximumPendingRequests) {
        setDefaultValue(maximumPendingRequestsOpt, maximumPendingRequests);
        return this;
    }

    std::size_t ConfigHttpServer::getMaximumPendingRequests() const {
        return maximumPendingRequestsOpt->as<std::size_t>();
    }

    ConfigHttpServer* ConfigHttpServer::setAllowChunkedTransfer(bool allowChunkedTransfer) {
        setDefaultValue(allowChunkedTransferOpt, allowChunkedTransfer ? "true" : "false");
        return this;
    }

    bool ConfigHttpServer::getAllowChunkedTransfer() const {
        return allowChunkedTransferOpt->as<bool>();
    }

    ConfigHttpServer* ConfigHttpServer::setAllowPipelining(bool allowPipelining) {
        setDefaultValue(allowPipeliningOpt, allowPipelining ? "true" : "false");
        return this;
    }

    bool ConfigHttpServer::getAllowPipelining() const {
        return allowPipeliningOpt->as<bool>();
    }

    web::http::ConfigHttpParser* ConfigHttpServer::getParserConfig() const {
        return getSubCommand<web::http::ConfigHttpParser>();
    }

    web::http::ParserLimits ConfigHttpServer::getParserLimits() const {
        return getParserConfig()->getParserLimits();
    }

    HttpServerPolicy ConfigHttpServer::getServerPolicy() const {
        return {
            .maximumPendingRequests = getMaximumPendingRequests(),
            .allowChunkedTransfer = getAllowChunkedTransfer(),
            .allowPipelining = getAllowPipelining(),
        };
    }

} // namespace web::http::server
