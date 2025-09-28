/*
 * SNode.C - A Slim Toolkit for Network Communication
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

#include "web/http/decoder/Chunked.h"

#include "core/socket/stream/SocketContext.h"

#include <stdexcept>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::decoder {

    Chunked::Chunked(const core::socket::stream::SocketContext* socketContext)
        : socketContext(socketContext) {
    }

    std::size_t Chunked::read() {
        std::size_t consumed = 0;
        std::size_t ret = 0;

        switch (state) {
            case -1:
                content.clear();
                completed = false;
                error = false;

                state = 0;

                [[fallthrough]];
            case 0:
                do {
                    ret = chunk.read(socketContext);
                    consumed += ret;

                    if (chunk.isComplete()) {
                        content.insert(content.end(), chunk.begin(), chunk.end());

                        completed = chunk.size() == 0;

                    } else if (chunk.isError()) {
                        error = true;
                    }

                    state = (completed || error) ? -1 : state;
                } while (ret > 0 && !completed);
                break;
        }

        return consumed;
    }

    Chunked::Chunk::~Chunk() {
    }

    inline std::size_t Chunked::Chunk::read(const core::socket::stream::SocketContext* socketContext) {
        std::size_t consumed = 0;
        std::size_t ret = 0;
        std::size_t pos = 0;

        const static int maxChunkLenTotalS = sizeof(std::size_t) * 2;

        switch (state) {
            case -1: // Re-init
                CR = false;
                LF = false;
                chunkLenTotalS.clear();
                chunkLenTotal = 0;
                chunkLenRead = 0;

                completed = false;
                error = false;

                state = 0;

                [[fallthrough]];
            case 0: // ChunklLenS
                do {
                    char ch = '\0';

                    ret = socketContext->readFromPeer(&ch, 1);
                    if (ret > 0) {
                        consumed++;

                        if (CR) {
                            if (ch == '\n') {
                                LF = true;
                            } else {
                                error = true;
                            }
                        } else if (ch == '\r') {
                            CR = true;
                        } else {
                            chunkLenTotalS += ch;
                        }
                    }
                } while (!error && ret > 0 && !(CR && LF) && chunkLenTotalS.size() <= maxChunkLenTotalS);

                if (!(CR && LF)) {
                    error = !error ? chunkLenTotalS.size() > maxChunkLenTotalS : error;

                    if (error) {
                        state = -1;
                    }

                    break;
                }

                CR = false;
                LF = false;

                try {
                    chunkLenTotal = std::stoul(chunkLenTotalS, &pos, 16);
                    chunk.resize(chunkLenTotal);

                    state = 1;
                } catch (std::invalid_argument&) {
                    error = true;
                    break;
                }

                [[fallthrough]];
            case 1: // ChunkData
                do {
                    ret = socketContext->readFromPeer(chunk.data() + chunkLenRead, chunkLenTotal - chunkLenRead);
                    chunkLenRead += ret;
                    consumed += ret;
                } while (ret > 0 && (chunkLenTotal != chunkLenRead));

                if (chunkLenTotal != chunkLenRead) {
                    break;
                }

                state = 2;

                [[fallthrough]];
            case 2: // Closing "\r\n"
                char ch = '\0';

                ret = socketContext->readFromPeer(&ch, 1);
                consumed += ret;

                if (ret > 0) {
                    if (CR || ch == '\r') {
                        if (CR && ch == '\n') {
                            LF = true;
                            completed = true;
                            state = -1;
                        } else if (CR) {
                            error = true;
                            state = -1;
                        } else {
                            CR = true;
                        }
                    } else {
                        error = true;
                        state = -1;
                    }
                }

                break;
        }

        return consumed;
    }

    inline bool Chunked::Chunk::isError() const {
        return error;
    }

    inline bool Chunked::Chunk::isComplete() const {
        return completed;
    }

    std::vector<char>::iterator Chunked::Chunk::begin() {
        return chunk.begin();
    }

    std::vector<char>::iterator Chunked::Chunk::end() {
        return chunk.end();
    }

    std::size_t Chunked::Chunk::size() {
        return chunk.size();
    }

} // namespace web::http::decoder
