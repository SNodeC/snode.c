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

#include "web/http/ConfigHttpParser.h"

#include <string>

namespace web::http {

    ConfigHttpParser::ConfigHttpParser(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Resource Policies") {
        maximumStartLineBytesOpt = addOption( //
            "--maximum-start-line-bytes",
            "Maximum HTTP start-line size in bytes (0 = unlimited)",
            "bytes",
            ParserLimits::DEFAULT_MAXIMUM_START_LINE_BYTES,
            CLI::NonNegativeNumber);
        maximumHeaderLineBytesOpt = addOption( //
            "--maximum-header-line-bytes",
            "Maximum HTTP header-line size in bytes (0 = unlimited)",
            "bytes",
            ParserLimits::DEFAULT_MAXIMUM_HEADER_LINE_BYTES,
            CLI::NonNegativeNumber);
        maximumHeaderBytesOpt = addOption( //
            "--maximum-header-bytes",
            "Maximum HTTP header section size in bytes (0 = unlimited)",
            "bytes",
            ParserLimits::DEFAULT_MAXIMUM_HEADER_BYTES,
            CLI::NonNegativeNumber);
        maximumHeaderFieldsOpt = addOption( //
            "--maximum-header-fields",
            "Maximum number of HTTP header fields (0 = unlimited)",
            "count",
            ParserLimits::DEFAULT_MAXIMUM_HEADER_FIELDS,
            CLI::NonNegativeNumber);
        maximumBodyBytesOpt = addOption( //
            "--maximum-body-bytes",
            "Maximum decoded HTTP message body size in bytes (0 = unlimited)",
            "bytes",
            ParserLimits::DEFAULT_MAXIMUM_BODY_BYTES,
            CLI::NonNegativeNumber);
    }

    ConfigHttpParser::~ConfigHttpParser() {
    }

    ConfigHttpParser* ConfigHttpParser::setMaximumStartLineBytes(std::size_t maximumStartLineBytes) {
        setDefaultValue(maximumStartLineBytesOpt, maximumStartLineBytes);
        return this;
    }

    std::size_t ConfigHttpParser::getMaximumStartLineBytes() const {
        return maximumStartLineBytesOpt->as<std::size_t>();
    }

    ConfigHttpParser* ConfigHttpParser::setMaximumHeaderLineBytes(std::size_t maximumHeaderLineBytes) {
        setDefaultValue(maximumHeaderLineBytesOpt, maximumHeaderLineBytes);
        return this;
    }

    std::size_t ConfigHttpParser::getMaximumHeaderLineBytes() const {
        return maximumHeaderLineBytesOpt->as<std::size_t>();
    }

    ConfigHttpParser* ConfigHttpParser::setMaximumHeaderBytes(std::size_t maximumHeaderBytes) {
        setDefaultValue(maximumHeaderBytesOpt, maximumHeaderBytes);
        return this;
    }

    std::size_t ConfigHttpParser::getMaximumHeaderBytes() const {
        return maximumHeaderBytesOpt->as<std::size_t>();
    }

    ConfigHttpParser* ConfigHttpParser::setMaximumHeaderFields(std::size_t maximumHeaderFields) {
        setDefaultValue(maximumHeaderFieldsOpt, maximumHeaderFields);
        return this;
    }

    std::size_t ConfigHttpParser::getMaximumHeaderFields() const {
        return maximumHeaderFieldsOpt->as<std::size_t>();
    }

    ConfigHttpParser* ConfigHttpParser::setMaximumBodyBytes(std::size_t maximumBodyBytes) {
        setDefaultValue(maximumBodyBytesOpt, maximumBodyBytes);
        return this;
    }

    std::size_t ConfigHttpParser::getMaximumBodyBytes() const {
        return maximumBodyBytesOpt->as<std::size_t>();
    }

    ParserLimits ConfigHttpParser::getParserLimits() const {
        return {
            .maximumStartLineBytes = getMaximumStartLineBytes(),
            .maximumHeaderLineBytes = getMaximumHeaderLineBytes(),
            .maximumHeaderBytes = getMaximumHeaderBytes(),
            .maximumHeaderFields = getMaximumHeaderFields(),
            .maximumBodyBytes = getMaximumBodyBytes(),
        };
    }

} // namespace web::http
