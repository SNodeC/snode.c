/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef WEB_HTTP_DECODER_HEADER_H
#define WEB_HTTP_DECODER_HEADER_H

namespace core::socket::stream {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/CiStringMap.h"

#include <cstddef>
#include <set>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::decoder {

    class Fields {
    public:
        explicit Fields(core::socket::stream::SocketContext* socketContext, std::set<std::string> fieldsExpected = {});
        Fields(Fields&) = delete;
        Fields(Fields&&) = delete;

        Fields& operator=(Fields&) = delete;
        Fields& operator=(Fields&&) = delete;

        void setFieldsExpected(std::set<std::string> fieldsExpected);

        std::size_t read();

        web::http::CiStringMap<std::string>&& getHeader();

        bool isComplete() const;
        bool isError() const;

        int getErrorCode() const;
        std::string getErrorReason();

    private:
        void splitLine(const std::string& line);

        core::socket::stream::SocketContext* socketContext;

        web::http::CiStringMap<std::string> fields;
        std::set<std::string> fieldsExpected;

        std::string line;
        std::size_t maxLineLength;

        bool completed = false;
        int errorCode = 0;
        std::string errorReason;

        char lastButOne = '\0';
        char last = '\0';
    };

} // namespace web::http::decoder

#endif // WEB_HTTP_DECODER_HEADER_H
