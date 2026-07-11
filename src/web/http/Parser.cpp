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

#include "web/http/Parser.h"

#include "core/socket/stream/SocketContext.h"
#include "web/http/decoder/Chunked.h"
#include "web/http/decoder/HTTP10Response.h"
#include "web/http/decoder/Identity.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/http_utils.h"

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

        for (const ContentDecoder* contentDecoder : decoderQueue) {
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
                        parseStartLine(line);
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
            parseError(headerDecoder.getErrorCode(), headerDecoder.getErrorReason());
        } else if (headerDecoder.isComplete()) {
            headers = std::move(headerDecoder.getHeader());
            analyzeHeader();
        }

        return consumed;
    }

    void Parser::useChunkedBodyDecoder() {
        transferEncoding = TransferEncoding::Chunked;
        decoderQueue.emplace_back(new web::http::decoder::Chunked(socketContext));
        configureTrailerDecoder();
    }

    void Parser::useIdentityBodyDecoder(std::size_t length) {
        contentLength = length;
        transferEncoding = TransferEncoding::Identity;
        decoderQueue.emplace_back(new web::http::decoder::Identity(socketContext, contentLength));
    }

    bool Parser::configureTrailerDecoder() {
        if (headers.contains("Trailer")) {
            for (std::string trailerField : httputils::splitCommaSeparatedTokens(headers["Trailer"])) {
                if (!trailerField.empty()) {
                    trailerFieldsExpected.insert(std::move(trailerField));
                }
            }
            trailerDecoder.setFieldsExpected(trailerFieldsExpected);
        }
        return true;
    }

    bool Parser::allowsCloseDelimitedBody() const {
        return true;
    }

    void Parser::analyzeHeader() {
        std::size_t length = 0;
        switch (httputils::parseContentLength(headers, length)) {
            case httputils::ContentLengthParseResult::Absent:
                break;
            case httputils::ContentLengthParseResult::Valid:
                useIdentityBodyDecoder(length);
                break;
            case httputils::ContentLengthParseResult::Invalid:
                parseError(400, "Invalid Content-Length");
                return;
        }

        if (headers.contains("Transfer-Encoding") && httputils::headerHasToken(headers["Transfer-Encoding"], "chunked")) {
            for (const ContentDecoder* contentDecoder : decoderQueue) {
                delete contentDecoder;
            }
            decoderQueue.clear();
            useChunkedBodyDecoder();
        }

        if (decoderQueue.empty() && allowsCloseDelimitedBody()) {
            decoderQueue.emplace_back(new web::http::decoder::HTTP10Response(socketContext));
        }
    }

    std::size_t Parser::readContent() {
        ContentDecoder* contentDecoder = decoderQueue.front();

        const std::size_t consumed = contentDecoder->read();

        if (contentDecoder->isComplete()) {
            contentDecoder = decoderQueue.back();

            std::vector<char> chunk = contentDecoder->getContent();
            content.insert(content.end(), chunk.begin(), chunk.end());

            if (transferEncoding == TransferEncoding::Chunked) {
                parserState = Parser::ParserState::TRAILER;
            } else {
                parsingFinished();
            }
        } else if (contentDecoder->isError()) {
            parseError(501, "Wrong content encoding");
        }

        return consumed;
    }

    std::size_t Parser::readTrailer() {
        const std::size_t consumed = trailerDecoder.read();

        if (trailerDecoder.isError()) {
            parseError(trailerDecoder.getErrorCode(), trailerDecoder.getErrorReason());
        } else if (trailerDecoder.isComplete()) {
            web::http::CiStringMap<std::string>&& trailer = trailerDecoder.getHeader();
            for (const auto& [field, value] : trailer) {
                if (!web::http::ciEquals(field, "Content-Length") && !web::http::ciEquals(field, "Transfer-Encoding") &&
                    !web::http::ciEquals(field, "Host") && !web::http::ciEquals(field, "Connection")) {
                    headers.insert({field, value});
                }
            }
            parsingFinished();
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
