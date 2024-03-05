/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef WEB_HTTP_CONTENTDECODER_H
#define WEB_HTTP_CONTENTDECODER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http {

    class ContentDecoder {
    public:
        ContentDecoder() = default;
        ContentDecoder(const ContentDecoder&) = delete;

        ContentDecoder(ContentDecoder&&) noexcept = default;

        ContentDecoder& operator=(const ContentDecoder&) = delete;

        ContentDecoder& operator=(ContentDecoder&&) noexcept = default;

        virtual ~ContentDecoder();

        virtual std::size_t read() = 0;

        bool isComplete() const;

        bool isError() const;

        std::vector<char>&& getContent();

    protected:
        std::vector<char> content;

        bool completed = false;
        bool error = false;
    };

} // namespace web::http

#endif // WEB_HTTP_CONTENTDECODER_H
