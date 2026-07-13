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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <openssl/err.h>
#include <openssl/ssl.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    void TLSHandshake::doHandshake(const std::string& instanceName,
                                   SSL* ssl,
                                   const std::function<void(void)>& onSuccess,
                                   const std::function<void(void)>& onTimeout,
                                   const std::function<void(int)>& onStatus,
                                   const utils::Timeval& timeout,
                                   const std::function<void(void)>& onReleased) {
        auto* helper = new TLSHandshake(instanceName, ssl, onSuccess, onTimeout, onStatus, timeout, onReleased);
        helper->start();
    }

    TLSHandshake::TLSHandshake(const std::string& instanceName,
                               SSL* ssl,
                               const std::function<void(void)>& onSuccess,
                               const std::function<void(void)>& onTimeout,
                               const std::function<void(int)>& onStatus,
                               const utils::Timeval& timeout,
                               const std::function<void(void)>& onReleased)
        : ReadEventReceiver(instanceName + " SSL/TLS: Handshake", timeout)
        , WriteEventReceiver(instanceName + " SSL/TLS: Handshake", timeout)
        , ssl(ssl)
        , onSuccess(onSuccess)
        , onTimeout(onTimeout)
        , onStatus(onStatus)
        , onReleased(onReleased)
        , fd(SSL_get_fd(ssl)) {
    }

    void TLSHandshake::start() {
        if (completed) {
            return;
        }

        const int sslErr = performOperation();

        switch (sslErr) {
            case SSL_ERROR_WANT_READ:
                awaitRead();
                break;
            case SSL_ERROR_WANT_WRITE:
                awaitWrite();
                break;
            case SSL_ERROR_NONE:
                finishSuccess();
                break;
            case SSL_ERROR_ZERO_RETURN:
            default:
                finishError(sslErr, errno);
                break;
        }
    }

    int TLSHandshake::performOperation() {
        if (completed) {
            return SSL_ERROR_NONE;
        }

        ERR_clear_error();
        errno = 0;
        const int ret = SSL_do_handshake(ssl);
        const int savedErrno = errno;
        int sslErr = SSL_ERROR_NONE;
        if (ret <= 0) {
            sslErr = SSL_get_error(ssl, ret);
        }
        errno = savedErrno;
        return ret == 1 ? SSL_ERROR_NONE : sslErr;
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
        const auto callback = onSuccess;
        disableRegisteredReceivers();
        callback();
        releaseAndDestroy();
    }

    void TLSHandshake::finishTimeout() {
        if (completed) {
            return;
        }
        completed = true;
        const auto callback = onTimeout;
        disableRegisteredReceivers();
        callback();
        releaseAndDestroy();
    }

    void TLSHandshake::finishError(int sslErr, int systemErr) {
        if (completed) {
            return;
        }
        completed = true;
        const auto callback = onStatus;
        disableRegisteredReceivers();
        if (systemErr != 0) {
            errno = systemErr;
        }
        callback(sslErr);
        releaseAndDestroy();
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

    void TLSHandshake::releaseAndDestroy() {
        if (!everObserved) {
            notifyReleased();
            delete this;
        }
    }

    void TLSHandshake::signalEvent([[maybe_unused]] int signum) { // Do nothing on signal event
    }

    void TLSHandshake::unobservedEvent() {
        notifyReleased();
        delete this;
    }

} // namespace core::socket::stream::tls
