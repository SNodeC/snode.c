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

#include "core/socket/stream/tls/SocketWriter.h"

#include "core/socket/stream/tls/detail/TLSResult.h"
#if defined(SNODEC_BUILD_TESTS)
#include "core/socket/stream/tls/detail/TLSLifecycleTestAccess.h"
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/SemanticLogger.h"
#include "utils/PreserveErrno.h"

#include <cerrno>
#include <deque>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string>
#include <variant>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#if defined(SNODEC_BUILD_TESTS)
namespace core::socket::stream::tls::detail::test {
    IoState& writerState() {
        static IoState state;
        return state;
    }
} // namespace core::socket::stream::tls::detail::test
#endif

namespace core::socket::stream::tls {

    SocketWriter::SocketWriter(const std::string& instanceName,
                               logger::LogScope streamLogScope,
                               const std::function<void(int)>& onStatus,
                               const utils::Timeval& timeout,
                               std::size_t blockSize,
                               const utils::Timeval& terminateTimeout,
                               std::size_t maximumWriteQueueBytes,
                               std::size_t writeQueueHighWatermark,
                               std::size_t writeQueueLowWatermark)
        : Super(instanceName,
                streamLogScope,
                onStatus,
                timeout,
                blockSize,
                terminateTimeout,
                maximumWriteQueueBytes,
                writeQueueHighWatermark,
                writeQueueLowWatermark) {
    }

    ssize_t SocketWriter::write(const char* chunk, std::size_t chunkLen) {
        ssize_t ret = 0;

        if (ssl == nullptr) {
            ret = Super::write(chunk, chunkLen);
        } else {
            detail::TlsIoResult result;
#if defined(SNODEC_BUILD_TESTS)
            auto& testState = detail::test::writerState();
            testState.counters.operationCalls++;
            if (!testState.operations.empty()) {
                const detail::test::OperationResult operation = testState.operations.front();
                testState.operations.pop_front();
                ret = operation.returnValue;
                errno = operation.systemError;
                result = ret > 0 ? detail::TlsIoResult{detail::TlsIoSuccess{ret}}
                                 : detail::TlsIoResult{detail::classifyOpenSslFailure(
                                       static_cast<int>(ret), operation.sslError, operation.systemError, operation.openSslError)};
            } else
#endif
            {
                ERR_clear_error();
                errno = 0;
                ret = SSL_write(ssl, chunk, static_cast<int>(chunkLen));
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
                        log().trace("SSL/TLS: Start renegotiation on write");
                        doSSLHandshake(
                            [log = this->log()]() {
                                log.debug("SSL/TLS: Renegotiation on write success");
                            },
                            [log = this->log()]() {
                                log.warn("SSL/TLS: Renegotiation on write timed out");
                            },
                            [this](int sslErr) {
                                ssl_log(log(), "SSL/TLS: Renegotiation on write", sslErr);
                            });
                        errno = EAGAIN;
                        ret = -1;
                        break;
                    case detail::TlsStatus::WantWrite:
                        errno = EAGAIN;
                        ret = -1;
                        break;
                    case detail::TlsStatus::CleanPeerShutdown:
                        errno = EAGAIN;
                        ret = -1;
                        break;
                    case detail::TlsStatus::UncleanEofWithoutCloseNotify: {
                        const int errnum = EPROTO;
                        log().error("SSL/TLS: Transport ended without TLS close_notify on write");
                        errno = errnum;
                        onTlsFatalError(errnum);
                        errno = errnum;
                        ret = -1;
                        break;
                    }
                    case detail::TlsStatus::SyscallError: {
                        const int errnum = detail::fatalTlsStatusToErrno(status);
                        const utils::PreserveErrno pe;
                        log().sysError(logger::LogLevel::Warn, errnum, "SSL/TLS: Syscall error on write");
                        errno = errnum;
                        onTlsFatalError(errnum);
                        errno = errnum;
                        ret = -1;
                        break;
                    }
                    case detail::TlsStatus::SslProtocolError: {
                        const int errnum = EPROTO;
                        ssl_log(log(), "SSL/TLS: Write protocol failure", status.sslError);
                        errno = errnum;
                        onTlsFatalError(errnum);
                        errno = errnum;
                        ret = -1;
                        break;
                    }
                    case detail::TlsStatus::UnknownError: {
                        const int errnum = detail::fatalTlsStatusToErrno(status);
                        ssl_log(log(), "SSL/TLS: Unknown write failure", status.sslError);
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

} // namespace core::socket::stream::tls
