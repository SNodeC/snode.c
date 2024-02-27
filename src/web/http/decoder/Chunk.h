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

#ifndef WEB_HTTP_DECODER_CHUNK_H
#define WEB_HTTP_DECODER_CHUNK_H

namespace core::socket::stream {
    class SocketContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::decoder {

    class Chunk { // Extends decoder
    private:
        class ChunkImpl {
        public:
            explicit ChunkImpl(core::socket::stream::SocketContext* socketContext)
                : socketContext(socketContext) {
            }

            ChunkImpl(const ChunkImpl&) = delete;
            ChunkImpl(ChunkImpl&&) noexcept = delete;

            ChunkImpl& operator=(const ChunkImpl&) = delete;
            ChunkImpl& operator=(ChunkImpl&&) noexcept = delete;

            ~ChunkImpl();

            std::size_t read();

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

            core::socket::stream::SocketContext* socketContext;
        };

    public:
        explicit Chunk(core::socket::stream::SocketContext* socketContext);

        std::size_t read();

        bool isCompleted() const;

        bool isError() const;

        std::vector<char> getContent();

    private:
        ChunkImpl chunkImpl;
        std::vector<char> content;

        bool completed = false;
        bool error = false;

        int state = 0;
    };

} // namespace web::http::decoder

#endif // WEB_HTTP_DECODER_CHUNK_H
