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

#include "core/socket/stream/tls/TLSHandshake.h"

#include "core/socket/stream/tls/detail/TLSResult.h"
#include "log/LogScopeOwner.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <variant>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    void TLSHandshake::doHandshake(const std::string& instanceName,
                                   SSL* ssl,
                                   const std::function<void(void)>& onSuccess,
                                   const std::function<void(void)>& onTimeout,
                                   const std::function<void(int)>& onStatus,
                                   const utils::Timeval& timeout) {
        const logger::LogScopeOwner logScope(
            logger::LogOrigin::Framework, logger::LogBoundary::System, "core.eventreceiver", instanceName + " SSL/TLS: Handshake");
        auto* helper = new TLSHandshake(instanceName, logScope.scope(), ssl, onSuccess, onTimeout, onStatus, timeout, {}, SSL_get_fd(ssl));
        helper->start();
    }

    void TLSHandshake::doHandshakeWithRelease(const std::string& instanceName,
                                              logger::LogScope logScope,
                                              SSL* ssl,
                                              const std::function<void(void)>& onSuccess,
                                              const std::function<void(void)>& onTimeout,
                                              const std::function<void(int)>& onStatus,
                                              const utils::Timeval& timeout,
                                              const std::function<void(void)>& onReleased) {
        auto* helper = new TLSHandshake(instanceName, logScope, ssl, onSuccess, onTimeout, onStatus, timeout, onReleased, SSL_get_fd(ssl));
        helper->start();
    }

    TLSHandshake::TLSHandshake(const std::string& instanceName,
                               logger::LogScope logScope,
                               SSL* ssl,
                               const std::function<void(void)>& onSuccess,
                               const std::function<void(void)>& onTimeout,
                               const std::function<void(int)>& onStatus,
                               const utils::Timeval& timeout,
                               const std::function<void(void)>& onReleased,
                               int fd)
        : ReadEventReceiver(instanceName + " SSL/TLS: Handshake", logScope, timeout)
        , WriteEventReceiver(instanceName + " SSL/TLS: Handshake", logScope, timeout)
        , ssl(ssl)
        , onSuccess(onSuccess)
        , onTimeout(onTimeout)
        , onStatus(onStatus)
        , onReleased(onReleased)
        , fd(fd) {
    }

    TLSHandshake::~TLSHandshake() {
    }

    void TLSHandshake::start() {
        if (completed) {
            return;
        }

        const detail::TlsHandshakeResult result = performOperation();

        if (std::holds_alternative<detail::TlsHandshakeSuccess>(result.value)) {
            finishSuccess();
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
            case detail::TlsStatus::UncleanEofWithoutCloseNotify:
            case detail::TlsStatus::SyscallError:
            case detail::TlsStatus::SslProtocolError:
            case detail::TlsStatus::UnknownError:
                finishError(status.sslError, status.status == detail::TlsStatus::CleanPeerShutdown ? EPROTO : detail::fatalTlsStatusToErrno(status));
                break;
        }
    }

    detail::TlsHandshakeResult TLSHandshake::performOperation() {
        if (completed) {
            return detail::TlsHandshakeResult{detail::TlsHandshakeSuccess{}};
        }

        ERR_clear_error();
        errno = 0;
        const int ret = SSL_do_handshake(ssl);
        const int savedErrno = errno;
        if (ret == 1) {
            errno = savedErrno;
            return detail::TlsHandshakeResult{detail::TlsHandshakeSuccess{}};
        }
        const int sslErr = SSL_get_error(ssl, ret);
        const unsigned long openSslError = ERR_peek_last_error();
        errno = savedErrno;
        return detail::TlsHandshakeResult{detail::classifyOpenSslFailure(ret, sslErr, savedErrno, openSslError)};
    }

    void TLSHandshake::awaitRead() {
        if (completed) {
            return;
        }
        if (writeRegistered && !WriteEventReceiver::isSuspended()) {
            WriteEventReceiver::suspend();
        }
        if (!readRegistered) {
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

    void TLSHandshake::awaitWrite() {
        if (completed) {
            return;
        }
        if (readRegistered && !ReadEventReceiver::isSuspended()) {
            ReadEventReceiver::suspend();
        }
        if (!writeRegistered) {
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

    void TLSHandshake::readEvent() {
        if (completed) {
            return;
        }
        start();
    }

    void TLSHandshake::writeEvent() {
        if (completed) {
            return;
        }
        start();
    }

    void TLSHandshake::finishSuccess() {
        if (completed) {
            return;
        }
        completed = true;
        const bool destroyImmediately = !everObserved;
        const auto callback = onSuccess;
        disableRegisteredReceivers();
        callback();
        if (destroyImmediately) {
            notifyReleased();
            delete this;
        }
    }

    void TLSHandshake::finishTimeout() {
        if (completed) {
            return;
        }
        completed = true;
        const bool destroyImmediately = !everObserved;
        const auto callback = onTimeout;
        disableRegisteredReceivers();
        callback();
        if (destroyImmediately) {
            notifyReleased();
            delete this;
        }
    }

    void TLSHandshake::finishError(int sslErr, int systemErr) {
        if (completed) {
            return;
        }
        completed = true;
        const bool destroyImmediately = !everObserved;
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

    void TLSHandshake::readTimeout() {
        if (completed) {
            return;
        }
        finishTimeout();
    }

    void TLSHandshake::writeTimeout() {
        if (completed) {
            return;
        }
        finishTimeout();
    }

    void TLSHandshake::disableRegisteredReceivers() {
        if (readRegistered && ReadEventReceiver::isEnabled()) {
            ReadEventReceiver::disable();
        }
        if (writeRegistered && WriteEventReceiver::isEnabled()) {
            WriteEventReceiver::disable();
        }
    }

    void TLSHandshake::notifyReleased() {
        if (!releaseNotified) {
            releaseNotified = true;
            if (onReleased) {
                onReleased();
            }
        }
    }

    void TLSHandshake::signalEvent([[maybe_unused]] int signum) { // Do nothing on signal event
    }

    void TLSHandshake::unobservedEvent() {
        notifyReleased();
        delete this;
    }

} // namespace core::socket::stream::tls
