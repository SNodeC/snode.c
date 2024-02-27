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

#include "web/http/decoder/Chunk.h"

#include "core/socket/stream/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::decoder {

    Chunk::Chunk(core::socket::stream::SocketContext* socketContext)
        : chunkImpl(socketContext) {
    }

    std::size_t Chunk::read() {
        std::size_t ret = 0;
        std::size_t consumed = 0;

        switch (state) {
            case -1:
                content.resize(0);
                completed = false;
                error = false;

                state = 0;

                [[fallthrough]];
            case 0:
                do {
                    ret = chunkImpl.read();
                    consumed += ret;

                    if (chunkImpl.isComplete()) {
                        content.insert(content.end(), chunkImpl.begin(), chunkImpl.end());

                        completed = chunkImpl.size() == 0;

                        state = completed ? -1 : 0;
                    }

                } while (ret > 0);
                break;
        }

        return consumed;
    }

    bool Chunk::isCompleted() const {
        return completed;
    }

    bool Chunk::isError() const {
        return error;
    }

    std::vector<char> Chunk::getContent() {
        return content;
    }

    Chunk::ChunkImpl::~ChunkImpl() {
    }

    std::size_t Chunk::ChunkImpl::read() {
        std::size_t consumed = 0;
        std::size_t ret = 0;
        std::size_t pos = 0;

        switch (state) {
            case -1:
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
                    consumed++;

                    if (CR) {
                        if (ch == '\n') {
                            LF = true;
                        } else {
                            chunkLenTotalS += '\r';
                            chunkLenTotalS += ch;

                            if (ch != '\r') {
                                CR = false;
                            }
                        }
                    } else if (ch == '\r') {
                        CR = true;
                    } else {
                        chunkLenTotalS += ch;
                    }
                } while (ret > 0 && !(CR && LF));

                if (!CR && !LF) {
                    break;
                }

                chunkLenTotal = std::stoul(chunkLenTotalS, &pos, 16);

                chunk.resize(chunkLenTotal);

                VLOG(0) << "#####: Length of ChunkLenTotalS: " << chunkLenTotalS.size() << " - processed: " << pos;
                VLOG(0) << "#####:            ChunkLenTotal: " << chunkLenTotal;

                state = 1;

                [[fallthrough]];
            case 1: // ChunkData
                do {
                    ret = socketContext->readFromPeer(chunk.data() + chunkLenRead, chunkLenTotal - chunkLenRead);
                    chunkLenRead += ret;
                    consumed += ret;
                } while (ret > 0 || chunkLenTotal == chunkLenRead);

                completed = chunkLenTotal == chunkLenRead;

                if (completed) {
                    state = -1;
                }

                break;
        }

        return consumed;
    }

    bool Chunk::ChunkImpl::isError() const {
        return error;
    }

    bool Chunk::ChunkImpl::isComplete() const {
        return completed;
    }

    std::vector<char>::iterator Chunk::ChunkImpl::begin() {
        return chunk.begin();
    }

    std::vector<char>::iterator Chunk::ChunkImpl::end() {
        return chunk.end();
    }

    std::size_t Chunk::ChunkImpl::size() {
        return chunk.size();
    }

} // namespace web::http::decoder
