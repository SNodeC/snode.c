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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#include <cerrno>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::tls {

    SocketWriter::SocketWriter(const std::string& instanceName,
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

    logger::BoundaryLogger SocketWriter::log() const {
        return logScope.logger(logger::Logger::semanticSink());
    }

    logger::BoundaryLogger
    SocketWriter::log(logger::BoundaryLogger::Sink sink, logger::LogLevel threshold, logger::BoundaryLogger::Clock clock) const {
        return logScope.logger(std::move(sink), threshold, std::move(clock));
    }

    ssize_t SocketWriter::write(const char* chunk, std::size_t chunkLen) {
        ssize_t ret = 0;

        if ((SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) != 0) {
            ret = Super::write(chunk, chunkLen);
        } else {
            ERR_clear_error();
            errno = 0;
            ret = SSL_write(ssl, chunk, static_cast<int>(chunkLen));
            const int savedErrno = errno;

            if (ret <= 0) {
                const int ssl_err = SSL_get_error(ssl, static_cast<int>(ret));
                [[maybe_unused]] const unsigned long openSslError = ERR_peek_last_error();
                errno = savedErrno;

                switch (ssl_err) {
                    case SSL_ERROR_WANT_READ:
                        log().trace("{} SSL/TLS: Start renegotiation on read", getName());
                        doSSLHandshake(
                            [log = this->log(), name = getName()]() {
                                log.debug("{} SSL/TLS: Renegotiation on read success", name);
                            },
                            [log = this->log(), name = getName()]() {
                                log.warn("{} SSL/TLS: Renegotiation on read timed out", name);
                            },
                            [this](int ssl_err) {
                                ssl_log(getName() + " SSL/TLS: Renegotiation", ssl_err);
                            });
                        errno = EAGAIN;
                        ret = -1;
                        break;
                    case SSL_ERROR_WANT_WRITE:
                        errno = EAGAIN;
                        ret = -1;
                        break;
                    case SSL_ERROR_ZERO_RETURN: // shutdown cleanly
                        errno = EAGAIN;
                        ret = -1; // on the write side this means a TCP broken pipe
                        break;
                    case SSL_ERROR_SYSCALL:
                        // In case ret is -1 a real syscall error (RST = ECONNRESET)
                        {
                            const int errnum = errno;
                            const utils::PreserveErrno pe;

                            if (errnum == EPIPE) {
                                log().sysError(
                                    logger::LogLevel::Warn, errnum, "{} SSL/TLS: Syscal error (SIGPIPE detected) on write.", getName());
                            } else if (errnum == ECONNRESET) {
                                log().sysError(
                                    logger::LogLevel::Warn, errnum, "{} SSL/TLS: Connection reset by peer (ECONNRESET).", getName());
                            } else {
                                log().sysError(logger::LogLevel::Warn, errnum, "{} SSL/TLS: Syscall error on write", getName());
                            }
                        }
                        ret = -1;
                        break;
                    case SSL_ERROR_SSL:
                        ssl_log(getName() + " SSL/TLS: Failed", ssl_err);
                        ret = -1;
                        break;
                    default:
                        ssl_log(getName() + " SSL/TLS: Unexpected error", ssl_err);
                        errno = EIO;
                        ret = -1;
                        break;
                }
            }
        }

        return ret;
    }

} // namespace core::socket::stream::tls
