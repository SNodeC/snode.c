/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef WEB_HTTP_DECODER_CHUNK_H
#define WEB_HTTP_DECODER_CHUNK_H

#include "web/http/ContentDecoder.h" // IWYU pragma: export

namespace core::socket::stream {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::decoder {

    class Chunked : public web::http::ContentDecoder { // Extends decoder
    private:
        class Chunk {
        public:
            Chunk() = default;

            Chunk(const Chunk&) = delete;
            Chunk(Chunk&&) noexcept = default;

            Chunk& operator=(const Chunk&) = delete;
            Chunk& operator=(Chunk&&) noexcept = default;

            ~Chunk();

            inline std::size_t read(const core::socket::stream::SocketContext* socketContext);

            bool isError() const;
            bool isComplete() const;

            std::vector<char>::iterator begin();
            std::vector<char>::iterator end();

            std::size_t size();

        private:
            std::vector<char> chunk;

            std::string chunkLenTotalS;
            std::size_t chunkLenTotal = 0;
            std::size_t chunkLenRead = 0;

            int state = 0;

            bool CR = false;
            bool LF = false;

            bool error = false;
            bool completed = false;
        };

    public:
        explicit Chunked(const core::socket::stream::SocketContext* socketContext);

        Chunked(const Chunked&) = delete;
        Chunked(Chunked&&) noexcept = default;

        Chunked& operator=(const Chunked&) = delete;
        Chunked& operator=(Chunked&&) noexcept = default;

    private:
        std::size_t read() override;

        const core::socket::stream::SocketContext* socketContext;

        Chunk chunk;

        int state = 0;
    };

} // namespace web::http::decoder

#endif // WEB_HTTP_DECODER_CHUNK_H
