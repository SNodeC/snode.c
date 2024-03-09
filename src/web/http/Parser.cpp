/*
 * SNode.C - a slim toolkit for network communication
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

#include "web/http/Parser.h"

#include "core/socket/stream/SocketContext.h"
#include "web/http/decoder/Chunked.h"
#include "web/http/decoder/HTTP10Response.h"
#include "web/http/decoder/Identity.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/http_utils.h"

#include <algorithm>
#include <tuple>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    // HTTP/1.0 and HTTP/1.1
    const std::regex Parser::httpVersionRegex("^HTTP/([1])[.]([0-1])$");

    Parser::Parser(core::socket::stream::SocketContext* socketContext, const enum Parser::HTTPCompliance& compliance)
        : hTTPCompliance(compliance)
        , socketContext(socketContext)
        , headerDecoder(socketContext)
        , trailerDecoder(socketContext) {
    }

    Parser::~Parser() {
        reset();
    }

    void Parser::reset() {
        parserState = ParserState::BEGIN;
        headers.clear();
        content.clear();
        httpMinor = 0;
        httpMajor = 0;
        line.clear();
        contentLength = 0;
        contentLengthRead = 0;

        for (ContentDecoder* contentDecoder : decoderQueue) {
            delete contentDecoder;
        }
        decoderQueue.clear();

        trailerFieldsExpected.clear();
    }

    std::size_t Parser::parse() {
        std::size_t ret = 0;
        std::size_t consumed = 0;

        do {
            switch (parserState) {
                case ParserState::BEGIN:
                    begin();
                    parserState = ParserState::FIRSTLINE;
                    [[fallthrough]];
                case ParserState::FIRSTLINE:
                    ret = readStartLine();
                    break;
                case ParserState::HEADER:
                    ret = readHeader();
                    break;
                case ParserState::BODY:
                    ret = readContent();
                    break;
                case ParserState::TRAILER:
                    ret = readTrailer();
                    break;
                case ParserState::ERROR:
                    break;
            }
            consumed += ret;
        } while (ret > 0 && parserState != ParserState::BEGIN && parserState != ParserState::ERROR);

        return consumed;
    }

    std::size_t Parser::readStartLine() {
        std::size_t consumed = 0;
        std::size_t ret = 0;

        do {
            char ch = 0;
            ret = socketContext->readFromPeer(&ch, 1);

            if (ret > 0) {
                consumed += ret;
                if (ch == '\r' || ch == '\n') {
                    if (ch == '\n') {
                        parserState = parseStartLine(line);
                        line.clear();
                    }
                } else {
                    line += ch;
                }
            }
        } while (ret > 0 && parserState == ParserState::FIRSTLINE);

        return consumed;
    }

    std::size_t Parser::readHeader() {
        const std::size_t consumed = headerDecoder.read();

        if (headerDecoder.isError()) {
            parserState = parseError(headerDecoder.getErrorCode(), headerDecoder.getErrorReason());
        } else if (headerDecoder.isComplete()) {
            headers = std::move(headerDecoder.getHeader());
            parserState = analyzeHeaders();
        }

        return consumed;
    }

    Parser::ParserState Parser::analyzeHeaders() {
        if (headers.contains("Content-Length")) {
            contentLength = std::stoul(headers["Content-Length"]);
            transferEncoding = TransferEncoding::Identity;
            decoderQueue.emplace_back(new web::http::decoder::Identity(socketContext, contentLength));
        }
        if (headers.contains("Transfer-Encoding")) {
            const std::string& encoding = headers["Transfer-Encoding"];

            if (web::http::ciContains(encoding, "chunked")) {
                transferEncoding = TransferEncoding::Chunked;
                decoderQueue.emplace_back(new web::http::decoder::Chunked(socketContext));
                if (headers.contains("Trailer")) {
                    std::string trailers = headers["Trailer"];

                    while (!trailers.empty()) {
                        std::string trailerField;
                        std::tie(trailerField, trailers) = httputils::str_split(trailers, ',');
                        httputils::str_trimm(trailerField);
                        trailerFieldsExpected.insert(trailerField);
                        trailerField.clear();
                    }
                    trailerDecoder.setFieldsExpected(trailerFieldsExpected);
                }
            }
            if (web::http::ciContains(encoding, "compressed")) {
                //  decoderQueue.emplace_back(new web::http::decoder::Compress(socketContext));
            }
            if (web::http::ciContains(encoding, "deflate")) {
                //  decoderQueue.emplace_back(new web::http::decoder::Deflate(socketContext));
            }
            if (web::http::ciContains(encoding, "gzip")) {
                //  decoderQueue.emplace_back(new web::http::decoder::GZip(socketContext));
            }
        }
        if (decoderQueue.empty()) {
            transferEncoding = TransferEncoding::HTTP10;
            decoderQueue.emplace_back(new web::http::decoder::HTTP10Response(socketContext));
        }

        if (headers.contains("Content-Encoding")) {
            const std::string& encoding = headers["Content-Encoding"];

            if (web::http::ciContains(encoding, "compressed")) {
                //  decoderQueue.emplace_back(new web::http::decoder::Compress(socketContext));
            }
            if (web::http::ciContains(encoding, "deflate")) {
                //  decoderQueue.emplace_back(new web::http::decoder::Deflate(socketContext));
            }
            if (web::http::ciContains(encoding, "gzip")) {
                //  decoderQueue.emplace_back(new web::http::decoder::GZip(socketContext));
            }
            if (web::http::ciContains(encoding, "br")) {
                //  decoderQueue.emplace_back(new web::http::decoder::Br(socketContext));
            }
        }

        return parseHeader();
    }

    std::size_t Parser::readContent() {
        ContentDecoder* contentDecoder = decoderQueue.front();

        const std::size_t consumed = contentDecoder->read();

        if (contentDecoder->isComplete()) {
            contentDecoder = decoderQueue.back();

            std::vector<char> chunk = contentDecoder->getContent();
            content.insert(content.end(), chunk.begin(), chunk.end());

            parserState = parseContent(content);
        } else if (contentDecoder->isError()) {
            parserState = parseError(400, "Wrong content encoding");
        }

        return consumed;
    }

    std::size_t Parser::readTrailer() {
        const std::size_t consumed = trailerDecoder.read();

        if (trailerDecoder.isError()) {
            parserState = parseError(trailerDecoder.getErrorCode(), trailerDecoder.getErrorReason());
        } else if (trailerDecoder.isComplete()) {
            parserState = parseTrailer(trailerDecoder.getHeader());
        }

        return consumed;
    }

    enum Parser::HTTPCompliance operator|(const enum Parser::HTTPCompliance& c1, const enum Parser::HTTPCompliance& c2) {
        return static_cast<enum Parser::HTTPCompliance>(static_cast<unsigned short>(c1) | static_cast<unsigned short>(c2));
    }

    enum Parser::HTTPCompliance operator&(const enum Parser::HTTPCompliance& c1, const enum Parser::HTTPCompliance& c2) {
        return static_cast<enum Parser::HTTPCompliance>(static_cast<unsigned short>(c1) & static_cast<unsigned short>(c2));
    }

} // namespace web::http
