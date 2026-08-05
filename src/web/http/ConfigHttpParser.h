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

#ifndef WEB_HTTP_CONFIGHTTPPARSER_H
#define WEB_HTTP_CONFIGHTTPPARSER_H

#include "utils/SubCommand.h"
#include "web/http/ParserLimits.h"

#include <cstddef>
#include <string_view>

namespace web::http {

    class ConfigHttpParser : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"parser"};
        constexpr static std::string_view DESCRIPTION{"HTTP parser resource limits"};

        explicit ConfigHttpParser(utils::SubCommand* parent);
        ~ConfigHttpParser() override;

        ConfigHttpParser(const ConfigHttpParser&) = delete;
        ConfigHttpParser(ConfigHttpParser&&) noexcept = delete;
        ConfigHttpParser& operator=(const ConfigHttpParser&) = delete;
        ConfigHttpParser& operator=(ConfigHttpParser&&) noexcept = delete;

        ConfigHttpParser* setMaximumStartLineBytes(std::size_t maximumStartLineBytes);
        std::size_t getMaximumStartLineBytes() const;

        ConfigHttpParser* setMaximumHeaderLineBytes(std::size_t maximumHeaderLineBytes);
        std::size_t getMaximumHeaderLineBytes() const;

        ConfigHttpParser* setMaximumHeaderBytes(std::size_t maximumHeaderBytes);
        std::size_t getMaximumHeaderBytes() const;

        ConfigHttpParser* setMaximumHeaderFields(std::size_t maximumHeaderFields);
        std::size_t getMaximumHeaderFields() const;

        ConfigHttpParser* setMaximumBodyBytes(std::size_t maximumBodyBytes);
        std::size_t getMaximumBodyBytes() const;

        ParserLimits getParserLimits() const;

    private:
        CLI::Option* maximumStartLineBytesOpt = nullptr;
        CLI::Option* maximumHeaderLineBytesOpt = nullptr;
        CLI::Option* maximumHeaderBytesOpt = nullptr;
        CLI::Option* maximumHeaderFieldsOpt = nullptr;
        CLI::Option* maximumBodyBytesOpt = nullptr;
    };

} // namespace web::http

#endif // WEB_HTTP_CONFIGHTTPPARSER_H
