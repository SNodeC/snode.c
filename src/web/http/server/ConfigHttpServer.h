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

#ifndef WEB_HTTP_SERVER_CONFIGHTTPSERVER_H
#define WEB_HTTP_SERVER_CONFIGHTTPSERVER_H

#include "utils/SubCommand.h"
#include "web/http/ParserLimits.h"

#include <cstddef>
#include <string_view>

namespace web::http {
    class ConfigHttpParser;
}

namespace web::http::server {

    struct HttpServerPolicy {
        std::size_t maximumPendingRequests = 0;
        bool allowChunkedTransfer = true;
        bool allowPipelining = true;
    };

    class ConfigHttpServer : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"http"};
        constexpr static std::string_view DESCRIPTION{"HTTP server behavior"};

        explicit ConfigHttpServer(utils::SubCommand* parent);
        ~ConfigHttpServer() override;

        ConfigHttpServer(const ConfigHttpServer&) = delete;
        ConfigHttpServer(ConfigHttpServer&&) noexcept = delete;
        ConfigHttpServer& operator=(const ConfigHttpServer&) = delete;
        ConfigHttpServer& operator=(ConfigHttpServer&&) noexcept = delete;

        ConfigHttpServer* setMaximumPendingRequests(std::size_t maximumPendingRequests);
        std::size_t getMaximumPendingRequests() const;

        ConfigHttpServer* setAllowChunkedTransfer(bool allowChunkedTransfer);
        bool getAllowChunkedTransfer() const;

        ConfigHttpServer* setAllowPipelining(bool allowPipelining);
        bool getAllowPipelining() const;

        web::http::ConfigHttpParser* getParserConfig() const;
        web::http::ParserLimits getParserLimits() const;
        HttpServerPolicy getServerPolicy() const;

    private:
        CLI::Option* maximumPendingRequestsOpt = nullptr;
        CLI::Option* allowChunkedTransferOpt = nullptr;
        CLI::Option* allowPipeliningOpt = nullptr;
    };

    using ConfigHTTP = ConfigHttpServer;

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_CONFIGHTTPSERVER_H
