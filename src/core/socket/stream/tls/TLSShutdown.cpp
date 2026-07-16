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

#include "core/socket/stream/tls/detail/TLSResult.h"

#if defined(SNODEC_BUILD_TESTS)
#include "core/socket/stream/tls/detail/TLSLifecycleTestAccess.h"
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cerrno>
#include <openssl/err.h>
#include <openssl/ssl.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#if defined(SNODEC_BUILD_TESTS)
namespace core::socket::stream::tls::detail::test {

    ShutdownState& shutdownState() {
        static ShutdownState state;
        return state;
    }


} // namespace core::socket::stream::tls::detail::test

#endif

namespace core::socket::stream::tls {

    void TLSShutdown::doShutdown(const std::string& instanceName,
                                 SSL* ssl,
                                 const std::function<void(void)>& onSuccess,
                                 const std::function<void(void)>& onTimeout,
                                 const std::function<void(int)>& onStatus,
                                 const utils::Timeval& timeout) {
        doShutdownWithRelease(instanceName, ssl, onSuccess, onTimeout, onStatus, timeout, {});
    }

    void TLSShutdown::doShutdownWithRelease(const std::string& instanceName,
                                            SSL* ssl,
                                            const std::function<void(void)>& onSuccess,
                                            const std::function<void(void)>& onTimeout,
                                            const std::function<void(int)>& onStatus,
                                            const utils::Timeval& timeout,
                                            const std::function<void(void)>& onReleased) {
        auto* helper = new TLSShutdown(
            instanceName,
            ssl,
            [onSuccess](TypedSuccess) {
                onSuccess();
            },
            onTimeout,
            onStatus,
            timeout,
            onReleased,
            SSL_get_fd(ssl));
        helper->start();
    }

    void TLSShutdown::doShutdownTypedWithRelease(const std::string& instanceName,
                                                 SSL* ssl,
                                                 const std::function<void(TypedSuccess)>& onSuccess,
                                                 const std::function<void(void)>& onTimeout,
                                                 const std::function<void(int)>& onStatus,
                                                 const utils::Timeval& timeout,
                                                 const std::function<void(void)>& onReleased,
                                                 CompletionRequirement completionRequirement) {
        auto* helper = new TLSShutdown(instanceName, ssl, onSuccess, onTimeout, onStatus, timeout, onReleased, SSL_get_fd(ssl));
        helper->completionRequirement = completionRequirement;
        helper->start();
    }

    TLSShutdown::TLSShutdown(const std::string& instanceName,
                             SSL* ssl,
                             const std::function<void(TypedSuccess)>& onSuccess,
                             const std::function<void(void)>& onTimeout,
                             const std::function<void(int)>& onStatus,
                             const utils::Timeval& timeout,
                             const std::function<void(void)>& onReleased,
                             int fd)
        : ReadEventReceiver(instanceName + " SSL/TLS: Send close_notify", timeout)
        , WriteEventReceiver(instanceName + " SSL/TLS: Send close_notify", timeout)
        , ssl(ssl)
        , onSuccess(onSuccess)
        , onTimeout(onTimeout)
        , onStatus(onStatus)
        , onReleased(onReleased)
        , fd(fd) {
#if defined(SNODEC_BUILD_TESTS)
        auto& state = detail::test::shutdownState();
        state.last = this;
        state.counters.constructed++;
        state.counters.active++;
        state.counters.maxConcurrent = std::max(state.counters.maxConcurrent, state.counters.active);
#endif
    }

    TLSShutdown::~TLSShutdown() {
#if defined(SNODEC_BUILD_TESTS)
        auto& state = detail::test::shutdownState();
        state.counters.destroyed++;
        state.counters.active--;
        if (state.last == this) {
            state.last = nullptr;
        }
#endif
    }

    void TLSShutdown::start() {
        if (completed) {
            return;
        }

        const detail::TlsShutdownResult result = performOperation();

        if (std::holds_alternative<detail::TlsShutdownSuccess>(result.value)) {
            switch (std::get<detail::TlsShutdownSuccess>(result.value)) {
                case detail::TlsShutdownSuccess::CloseNotifySent:
                    lastSuccess = TypedSuccess::CloseNotifySent;
#if defined(SNODEC_BUILD_TESTS)
                    detail::test::shutdownState().lastSuccess = detail::TlsShutdownSuccess::CloseNotifySent;
#endif
                    if (completionRequirement == CompletionRequirement::RequireFullShutdown) {
                        awaitRead();
                    } else {
                        finishSuccess();
                    }
                    break;
                case detail::TlsShutdownSuccess::FullShutdownComplete:
                    lastSuccess = TypedSuccess::FullShutdownComplete;
#if defined(SNODEC_BUILD_TESTS)
                    detail::test::shutdownState().lastSuccess = detail::TlsShutdownSuccess::FullShutdownComplete;
#endif
                    finishSuccess();
                    break;
            }
            return;
        }

        const detail::TlsStatusInfo& status = std::get<detail::TlsStatusInfo>(result.value);
        switch (status.status) {
            case detail::TlsStatus::WantRead:
                awaitRead();
                break;
            case detail::TlsStatus::WantWrite:
                awaitWrite();
                break;
            case detail::TlsStatus::CleanPeerShutdown:
                finishError(status.sslError, EPROTO);
                break;
            case detail::TlsStatus::UncleanEofWithoutCloseNotify:
            case detail::TlsStatus::SyscallError:
            case detail::TlsStatus::SslProtocolError:
                finishError(status.sslError, detail::fatalTlsStatusToErrno(status));
                break;
            case detail::TlsStatus::UnknownError:
                finishError(status.sslError, EPROTO);
                break;
        }
    }

    detail::TlsShutdownResult TLSShutdown::performOperation() {
        if (completed) {
            return detail::TlsShutdownResult{detail::TlsShutdownSuccess::FullShutdownComplete};
        }

#if defined(SNODEC_BUILD_TESTS)
        auto& state = detail::test::shutdownState();
        state.counters.operationCalls++;
        if (!state.operations.empty()) {
            const detail::test::OperationResult result = state.operations.front();
            state.operations.pop_front();
            errno = result.systemError;
            if (result.sslError == SSL_ERROR_NONE) {
                return detail::TlsShutdownResult{result.returnValue == 0 ? detail::TlsShutdownSuccess::CloseNotifySent
                                                                         : detail::TlsShutdownSuccess::FullShutdownComplete};
            }
            return detail::TlsShutdownResult{
                detail::classifyOpenSslFailure(result.returnValue, result.sslError, result.systemError, result.openSslError)};
        }
#endif

        ERR_clear_error();
        errno = 0;
        const int ret = SSL_shutdown(ssl);
        const int savedErrno = errno;
        if (ret == 0) {
            errno = savedErrno;
            return detail::TlsShutdownResult{detail::TlsShutdownSuccess::CloseNotifySent};
        }
        if (ret == 1) {
            errno = savedErrno;
            return detail::TlsShutdownResult{detail::TlsShutdownSuccess::FullShutdownComplete};
        }
        const int sslErr = SSL_get_error(ssl, ret);
        const unsigned long openSslError = ERR_peek_last_error();
        errno = savedErrno;
        return detail::TlsShutdownResult{detail::classifyOpenSslFailure(ret, sslErr, savedErrno, openSslError)};
    }

    void TLSShutdown::awaitRead() {
        if (completed) {
            return;
        }
        if (writeRegistered && !WriteEventReceiver::isSuspended()) {
            WriteEventReceiver::suspend();
        }
        if (!readRegistered) {
#if defined(SNODEC_BUILD_TESTS)
            auto& state = detail::test::shutdownState();
            if (state.failNextReadEnable != 0) {
                const int registrationErrno = state.failNextReadEnable;
                state.failNextReadEnable = 0;
                finishError(SSL_ERROR_SYSCALL, registrationErrno);
                return;
            }
#endif
            if (!ReadEventReceiver::enable(fd)) {
                const int registrationErrno = errno;
                finishError(SSL_ERROR_SYSCALL, registrationErrno == 0 ? EIO : registrationErrno);
                return;
            }
            readRegistered = true;
            everObserved = true;
        }
        if (ReadEventReceiver::isSuspended()) {
            ReadEventReceiver::resume();
        }
    }

    void TLSShutdown::awaitWrite() {
        if (completed) {
            return;
        }
        if (readRegistered && !ReadEventReceiver::isSuspended()) {
            ReadEventReceiver::suspend();
        }
        if (!writeRegistered) {
#if defined(SNODEC_BUILD_TESTS)
            auto& state = detail::test::shutdownState();
            if (state.failNextWriteEnable != 0) {
                const int registrationErrno = state.failNextWriteEnable;
                state.failNextWriteEnable = 0;
                finishError(SSL_ERROR_SYSCALL, registrationErrno);
                return;
            }
#endif
            if (!WriteEventReceiver::enable(fd)) {
                const int registrationErrno = errno;
                finishError(SSL_ERROR_SYSCALL, registrationErrno == 0 ? EIO : registrationErrno);
                return;
            }
            writeRegistered = true;
            everObserved = true;
        }
        if (WriteEventReceiver::isSuspended()) {
            WriteEventReceiver::resume();
        }
    }

    void TLSShutdown::readEvent() {
        if (completed) {
            return;
        }
        start();
    }

    void TLSShutdown::writeEvent() {
        if (completed) {
            return;
        }
        start();
    }

    void TLSShutdown::finishSuccess() {
        if (completed) {
            return;
        }
        completed = true;
        const bool destroyImmediately = !everObserved;
#if defined(SNODEC_BUILD_TESTS)
        detail::test::shutdownState().counters.successes++;
#endif
        const auto callback = onSuccess;
        const auto success = lastSuccess;
        disableRegisteredReceivers();
        callback(success);
        if (destroyImmediately) {
            notifyReleased();
            delete this;
        }
    }

    void TLSShutdown::finishTimeout() {
        if (completed) {
            return;
        }
        completed = true;
        const bool destroyImmediately = !everObserved;
#if defined(SNODEC_BUILD_TESTS)
        detail::test::shutdownState().counters.timeouts++;
#endif
        const auto callback = onTimeout;
        disableRegisteredReceivers();
        callback();
        if (destroyImmediately) {
            notifyReleased();
            delete this;
        }
    }

    void TLSShutdown::finishError(int sslErr, int systemErr) {
        if (completed) {
            return;
        }
        completed = true;
        const bool destroyImmediately = !everObserved;
#if defined(SNODEC_BUILD_TESTS)
        auto& state = detail::test::shutdownState();
        state.counters.errors++;
        state.counters.lastStatus = sslErr;
        state.counters.lastErrno = systemErr;
#endif
        const auto callback = onStatus;
        disableRegisteredReceivers();
        if (systemErr != 0) {
            errno = systemErr;
        }
        callback(sslErr);
        if (destroyImmediately) {
            notifyReleased();
            delete this;
        }
    }

    void TLSShutdown::readTimeout() {
        if (completed) {
            return;
        }
        finishTimeout();
    }

    void TLSShutdown::writeTimeout() {
        if (completed) {
            return;
        }
        finishTimeout();
    }

    void TLSShutdown::disableRegisteredReceivers() {
        if (readRegistered && ReadEventReceiver::isEnabled()) {
            ReadEventReceiver::disable();
        }
        if (writeRegistered && WriteEventReceiver::isEnabled()) {
            WriteEventReceiver::disable();
        }
    }

    void TLSShutdown::notifyReleased() {
        if (!releaseNotified) {
            releaseNotified = true;
#if defined(SNODEC_BUILD_TESTS)
            auto& state = detail::test::shutdownState();
            state.counters.releases++;
#endif
            if (onReleased) {
                onReleased();
            }
        }
    }

    void TLSShutdown::signalEvent([[maybe_unused]] int signum) { // Do nothing on signal event
    }

    void TLSShutdown::unobservedEvent() {
        notifyReleased();
        delete this;
    }

} // namespace core::socket::stream::tls
