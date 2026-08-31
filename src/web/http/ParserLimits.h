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

#ifndef WEB_HTTP_PARSERLIMITS_H
#define WEB_HTTP_PARSERLIMITS_H

#include <cstddef>

namespace web::http {

    struct ParserLimits {
        static constexpr std::size_t DEFAULT_MAXIMUM_START_LINE_BYTES = 0;
        static constexpr std::size_t DEFAULT_MAXIMUM_HEADER_LINE_BYTES = 8192;
        static constexpr std::size_t DEFAULT_MAXIMUM_HEADER_BYTES = 0;
        static constexpr std::size_t DEFAULT_MAXIMUM_HEADER_FIELDS = 0;
        static constexpr std::size_t DEFAULT_MAXIMUM_BODY_BYTES = 0;

        std::size_t maximumStartLineBytes = DEFAULT_MAXIMUM_START_LINE_BYTES;
        std::size_t maximumHeaderLineBytes = DEFAULT_MAXIMUM_HEADER_LINE_BYTES;
        std::size_t maximumHeaderBytes = DEFAULT_MAXIMUM_HEADER_BYTES;
        std::size_t maximumHeaderFields = DEFAULT_MAXIMUM_HEADER_FIELDS;
        std::size_t maximumBodyBytes = DEFAULT_MAXIMUM_BODY_BYTES;
    };

} // namespace web::http

#endif // WEB_HTTP_PARSERLIMITS_H
