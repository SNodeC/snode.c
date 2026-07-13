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

#include "core/socket/stream/tls/TLSShutdown.h"
#include "core/socket/stream/tls/detail/TLSShutdownPolicy.h"
#include "core/socket/stream/tls/detail/TLSShutdownTestHooks.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cerrno>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <optional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    namespace {
        std::size_t constructedCount = 0;
        std::size_t activeCount = 0;
        std::size_t maxActiveCount = 0;
        std::optional<detail::test::ForcedShutdownResult> forcedResult;

        struct ShutdownAttempt {
            int ret = -1;
            int sslErr = SSL_ERROR_NONE;
            int savedErrno = 0;
            unsigned long openSslError = 0;
        };

        ShutdownAttempt performShutdown(SSL* ssl) {
            if (forcedResult.has_value()) {
                ShutdownAttempt attempt{forcedResult->ret, forcedResult->sslError, forcedResult->systemError, forcedResult->openSslError};
                forcedResult.reset();
                return attempt;
            }

            ERR_clear_error();
            errno = 0;
            const int ret = SSL_shutdown(ssl);
            const int savedErrno = errno;

            int sslErr = SSL_ERROR_NONE;
            if (ret < 0) {
                sslErr = SSL_get_error(ssl, ret);
            }

            return {ret, sslErr, savedErrno, ret < 0 ? ERR_peek_last_error() : 0};
        }
    } // namespace

    void TLSShutdown::doShutdown(const std::string& instanceName,
                                 SSL* ssl,
                                 const std::function<void(void)>& onSuccess,
                                 const std::function<void(void)>& onTimeout,
                                 const std::function<void(int, int)>& onStatus,
                                 const utils::Timeval& timeout) {
        (new TLSShutdown(instanceName, ssl, onSuccess, onTimeout, onStatus, timeout))->start();
    }

    TLSShutdown::TLSShutdown(const std::string& instanceName,
                             SSL* ssl,
                             const std::function<void(void)>& onSuccess,
                             const std::function<void(void)>& onTimeout,
                             const std::function<void(int, int)>& onStatus,
                             const utils::Timeval& timeout)
        : ReadEventReceiver(instanceName + " SSL/TLS: Send close_notify", timeout)
        , WriteEventReceiver(instanceName + " SSL/TLS: Send close_notify", timeout)
        , ssl(ssl)
        , onSuccess(onSuccess)
        , onTimeout(onTimeout)
        , onStatus(onStatus)
        , fd(SSL_get_fd(ssl)) {
        constructedCount++;
        activeCount++;
        maxActiveCount = std::max(maxActiveCount, activeCount);
    }

    void TLSShutdown::start() {
        const ShutdownAttempt attempt = performShutdown(ssl);

        switch (classifyTlsShutdownResult(attempt.ret, attempt.sslErr, attempt.savedErrno, attempt.openSslError)) {
            case TlsShutdownClassification::FullShutdownComplete:
            case TlsShutdownClassification::CloseNotifySent:
                finishSuccess();
                return;
            case TlsShutdownClassification::WantRead:
                if (fd < 0) {
                    finishError(attempt.sslErr, attempt.savedErrno == 0 ? EBADF : attempt.savedErrno);
                    return;
                }
                if (!ReadEventReceiver::enable(fd)) {
                    finishError(attempt.sslErr, errno == 0 ? EBADF : errno);
                    return;
                }
                ReadEventReceiver::resume();
                return;
            case TlsShutdownClassification::WantWrite:
                if (fd < 0) {
                    finishError(attempt.sslErr, attempt.savedErrno == 0 ? EBADF : attempt.savedErrno);
                    return;
                }
                if (!WriteEventReceiver::enable(fd)) {
                    finishError(attempt.sslErr, errno == 0 ? EBADF : errno);
                    return;
                }
                WriteEventReceiver::resume();
                return;
            default:
                finishError(attempt.sslErr, attempt.savedErrno == 0 ? EIO : attempt.savedErrno);
                return;
        }
    }

    void TLSShutdown::readEvent() {
        const ShutdownAttempt attempt = performShutdown(ssl);

        switch (classifyTlsShutdownResult(attempt.ret, attempt.sslErr, attempt.savedErrno, attempt.openSslError)) {
            case TlsShutdownClassification::WantRead:
                break;
            case TlsShutdownClassification::WantWrite:
                ReadEventReceiver::disable();
                if (fd < 0) {
                    finishError(attempt.sslErr, attempt.savedErrno == 0 ? EBADF : attempt.savedErrno);
                    return;
                }
                if (!WriteEventReceiver::enable(fd)) {
                    finishError(attempt.sslErr, errno == 0 ? EBADF : errno);
                }
                break;
            case TlsShutdownClassification::FullShutdownComplete:
            case TlsShutdownClassification::CloseNotifySent:
                finishSuccess();
                break;
            default:
                finishError(attempt.sslErr, attempt.savedErrno == 0 ? EIO : attempt.savedErrno);
                break;
        }
    }

    void TLSShutdown::writeEvent() {
        const ShutdownAttempt attempt = performShutdown(ssl);

        switch (classifyTlsShutdownResult(attempt.ret, attempt.sslErr, attempt.savedErrno, attempt.openSslError)) {
            case TlsShutdownClassification::WantRead:
                WriteEventReceiver::disable();
                if (fd < 0) {
                    finishError(attempt.sslErr, attempt.savedErrno == 0 ? EBADF : attempt.savedErrno);
                    return;
                }
                if (!ReadEventReceiver::enable(fd)) {
                    finishError(attempt.sslErr, errno == 0 ? EBADF : errno);
                }
                break;
            case TlsShutdownClassification::WantWrite:
                break;
            case TlsShutdownClassification::FullShutdownComplete:
            case TlsShutdownClassification::CloseNotifySent:
                finishSuccess();
                break;
            default:
                finishError(attempt.sslErr, attempt.savedErrno == 0 ? EIO : attempt.savedErrno);
                break;
        }
    }

    void TLSShutdown::disableReceivers() {
        if (ReadEventReceiver::isEnabled()) {
            ReadEventReceiver::disable();
        }
        if (WriteEventReceiver::isEnabled()) {
            WriteEventReceiver::disable();
        }
    }

    void TLSShutdown::finishSuccess() {
        if (completed) {
            return;
        }
        completed = true;
        disableReceivers();
        const auto callback = std::move(onSuccess);
        activeCount--;
        callback();
        delete this;
    }

    void TLSShutdown::finishTimeout() {
        if (completed) {
            return;
        }
        completed = true;
        disableReceivers();
        const auto callback = std::move(onTimeout);
        activeCount--;
        callback();
        delete this;
    }

    void TLSShutdown::finishError(int sslError, int systemError) {
        if (completed) {
            return;
        }
        completed = true;
        disableReceivers();
        const auto callback = std::move(onStatus);
        activeCount--;
        callback(sslError, systemError);
        delete this;
    }

    void TLSShutdown::readTimeout() {
        finishTimeout();
    }

    void TLSShutdown::writeTimeout() {
        finishTimeout();
    }

    void TLSShutdown::signalEvent([[maybe_unused]] int signum) { // Do nothing on signal event
    }

    void TLSShutdown::unobservedEvent() {
        if (!completed) {
            completed = true;
            activeCount--;
        }
        delete this;
    }

} // namespace core::socket::stream::tls

namespace core::socket::stream::tls::detail::test {

    void resetTlsShutdownCounters() {
        constructedCount = 0;
        activeCount = 0;
        maxActiveCount = 0;
        forcedResult.reset();
    }

    std::size_t tlsShutdownConstructedCount() {
        return constructedCount;
    }

    std::size_t tlsShutdownActiveCount() {
        return activeCount;
    }

    std::size_t tlsShutdownMaxActiveCount() {
        return maxActiveCount;
    }

    void forceNextTlsShutdownResult(const ForcedShutdownResult& result) {
        forcedResult = result;
    }

    void clearForcedTlsShutdownResult() {
        forcedResult.reset();
    }

} // namespace core::socket::stream::tls::detail::test
