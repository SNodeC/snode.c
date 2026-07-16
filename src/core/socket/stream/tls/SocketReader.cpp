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

#include "core/socket/stream/tls/SocketReader.h"
#include "core/socket/stream/tls/detail/TLSResult.h"
#if defined(SNODEC_BUILD_TESTS)
#include "core/socket/stream/tls/detail/TLSLifecycleTestAccess.h"
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#include <algorithm>
#include <cerrno>
#include <limits>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS


#if defined(SNODEC_BUILD_TESTS)
namespace core::socket::stream::tls::detail::test {
    IoState& readerState() {
        static IoState state;
        return state;
    }
}
#endif

namespace core::socket::stream::tls {

    SocketReader::SocketReader(const std::string& instanceName,
                               const std::function<void(int)>& onStatus,
                               const utils::Timeval& timeout,
                               std::size_t blockSize,
                               const utils::Timeval& terminateTimeout)
        : Super(instanceName, onStatus, timeout, blockSize, terminateTimeout)
        , logScope(logger::LogOrigin::Framework,
                   logger::LogBoundary::Connection,
                   "core.socket.stream.tls",
                   instanceName.empty() ? std::nullopt : std::optional<std::string>(instanceName),
                   std::nullopt,
                   instanceName.empty() ? std::nullopt : std::optional<std::string>(instanceName)) {
    }

    logger::BoundaryLogger SocketReader::log() const {
        return logScope.logger(logger::Logger::semanticSink());
    }

    logger::BoundaryLogger
    SocketReader::log(logger::BoundaryLogger::Sink sink, logger::LogLevel threshold, logger::BoundaryLogger::Clock clock) const {
        return logScope.logger(std::move(sink), threshold, std::move(clock));
    }


    ssize_t SocketReader::read(char* chunk, std::size_t chunkLen) {
        if (handoffCursor < handoffBuffer.size()) {
            const std::size_t available = std::min(chunkLen, handoffBuffer.size() - handoffCursor);
            std::copy(handoffBuffer.data() + handoffCursor, handoffBuffer.data() + handoffCursor + available, chunk);
            handoffCursor += available;
            if (handoffCursor == handoffBuffer.size()) {
                handoffBuffer.clear();
                handoffCursor = 0;
            }
            return static_cast<ssize_t>(available);
        }

        ssize_t ret = 0;

        if (ssl == nullptr) {
            ret = Super::read(chunk, chunkLen);
        } else {
            chunkLen = chunkLen > std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : chunkLen;
            detail::TlsIoResult result;
#if defined(SNODEC_BUILD_TESTS)
            auto& testState = detail::test::readerState();
            testState.counters.operationCalls++;
            if (!testState.operations.empty()) {
                const detail::test::OperationResult operation = testState.operations.front();
                testState.operations.pop_front();
                ret = operation.returnValue;
                errno = operation.systemError;
                result = ret > 0 ? detail::TlsIoResult{detail::TlsIoSuccess{ret}}
                                 : detail::TlsIoResult{detail::classifyOpenSslFailure(static_cast<int>(ret), operation.sslError, operation.systemError, operation.openSslError)};
            } else
#endif
            {
                ERR_clear_error();
                errno = 0;
                ret = SSL_read(ssl, chunk, static_cast<int>(chunkLen));
                const int savedErrno = errno;

                if (ret > 0) {
                    result = detail::TlsIoResult{detail::TlsIoSuccess{ret}};
                } else {
                    const int sslErr = SSL_get_error(ssl, static_cast<int>(ret));
                    const unsigned long openSslError = ERR_peek_last_error();
                    result = detail::TlsIoResult{detail::classifyOpenSslFailure(static_cast<int>(ret), sslErr, savedErrno, openSslError)};
                }
            }

            if (const auto* success = std::get_if<detail::TlsIoSuccess>(&result.value)) {
                ret = success->bytesTransferred;
            } else {
                const detail::TlsStatusInfo& status = std::get<detail::TlsStatusInfo>(result.value);
                switch (status.status) {
                    case detail::TlsStatus::WantRead:
                        errno = EAGAIN;
                        ret = -1;
                        break;
                    case detail::TlsStatus::WantWrite:
                        log().trace("{} SSL/TLS: Start renegotiation on read", getName());
                        doSSLHandshake(
                            [log = this->log(), name = getName()]() {
                                log.debug("{} SSL/TLS: Renegotiation on read success", name);
                            },
                            [log = this->log(), name = getName()]() {
                                log.warn("{} SSL/TLS: Renegotiation on read timed out", name);
                            },
                            [this](int sslErr) {
                                ssl_log(getName() + " SSL/TLS: Renegotiation on read", sslErr);
                            });
                        errno = EAGAIN;
                        ret = -1;
                        break;
                    case detail::TlsStatus::CleanPeerShutdown:
                        log().debug("{} SSL/TLS: Clean peer close_notify on read", getName());
                        onReadShutdown();
                        errno = EAGAIN;
                        ret = -1;
                        break;
                    case detail::TlsStatus::UncleanEofWithoutCloseNotify: {
                        const int errnum = EPROTO;
                        log().error("{} SSL/TLS: Transport ended without TLS close_notify", getName());
                        errno = errnum;
                        onTlsFatalError(errnum);
                        errno = errnum;
                        ret = -1;
                        break;
                    }
                    case detail::TlsStatus::SyscallError: {
                        const int errnum = detail::fatalTlsStatusToErrno(status);
                        const utils::PreserveErrno pe;
                        log().sysError(logger::LogLevel::Warn, errnum, "{} SSL/TLS: Syscall error on read", getName());
                        errno = errnum;
                        onTlsFatalError(errnum);
                        errno = errnum;
                        ret = -1;
                        break;
                    }
                    case detail::TlsStatus::SslProtocolError: {
                        const int errnum = EPROTO;
                        ssl_log(getName() + " SSL/TLS: Read protocol failure", status.sslError);
                        errno = errnum;
                        onTlsFatalError(errnum);
                        errno = errnum;
                        ret = -1;
                        break;
                    }
                    case detail::TlsStatus::UnknownError: {
                        const int errnum = detail::fatalTlsStatusToErrno(status);
                        ssl_log(getName() + " SSL/TLS: Unknown read failure", status.sslError);
                        errno = errnum;
                        onTlsFatalError(errnum);
                        errno = errnum;
                        ret = -1;
                        break;
                    }
                }
            }
        }

        return ret;
    }

    void SocketReader::appendHandoffBytes(const char* data, std::size_t size) {
        if (data != nullptr && size > 0) {
            handoffBuffer.insert(handoffBuffer.end(), data, data + size);
        }
    }

} // namespace core::socket::stream::tls
