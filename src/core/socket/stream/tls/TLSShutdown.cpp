/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "core/socket/stream/tls/TLSShutdown.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/ssl.h> // IWYU pragma: keep

// IWYU pragma: no_include <openssl/ssl3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define TLSSHUTDOWN_TIMEOUT 10

namespace core::socket::stream::tls {

    void TLSShutdown::doShutdown(SSL* ssl,
                                 const std::function<void(void)>& onSuccess,
                                 const std::function<void(void)>& onTimeout,
                                 const std::function<void(int err)>& onError) {
        new TLSShutdown(ssl, onSuccess, onTimeout, onError);
    }

    TLSShutdown::TLSShutdown(SSL* ssl,
                             const std::function<void(void)>& onSuccess,
                             const std::function<void(void)>& onTimeout,
                             const std::function<void(int err)>& onError)
        : ReadEventReceiver(TLSSHUTDOWN_TIMEOUT)
        , WriteEventReceiver(TLSSHUTDOWN_TIMEOUT)
        , ssl(ssl)
        , onSuccess(onSuccess)
        , onTimeout(onTimeout)
        , onError(onError)
        , timeoutTriggered(false) {
        fd = SSL_get_fd(ssl);

        ReadEventReceiver::enable(fd);
        WriteEventReceiver::enable(fd);
        ReadEventReceiver::suspend();
        WriteEventReceiver::suspend();

        int ret = SSL_shutdown(ssl);

        int sslErr = SSL_ERROR_NONE;
        if (ret < 0) {
            sslErr = SSL_get_error(ssl, ret);
        }

        switch (sslErr) {
            case SSL_ERROR_WANT_READ:
                ReadEventReceiver::resume();
                break;
            case SSL_ERROR_WANT_WRITE:
                WriteEventReceiver::resume();
                break;
            case SSL_ERROR_NONE:
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                onSuccess();
                break;
            default:
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                onError(sslErr);
                break;
        }
    }

    void TLSShutdown::readEvent() {
        int ret = SSL_shutdown(ssl);

        int sslErr = SSL_ERROR_NONE;
        if (ret < 0) {
            sslErr = SSL_get_error(ssl, ret);
        }

        switch (sslErr) {
            case SSL_ERROR_WANT_READ:
                break;
            case SSL_ERROR_WANT_WRITE:
                ReadEventReceiver::suspend();
                WriteEventReceiver::resume();
                break;
            case SSL_ERROR_NONE:
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                onSuccess();
                break;
            default:
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                onError(sslErr);
                break;
        }
    }

    void TLSShutdown::writeEvent() {
        int ret = SSL_shutdown(ssl);

        int sslErr = SSL_ERROR_NONE;
        if (ret < 0) {
            sslErr = SSL_get_error(ssl, ret);
        }

        switch (sslErr) {
            case SSL_ERROR_WANT_READ:
                WriteEventReceiver::suspend();
                ReadEventReceiver::resume();
                break;
            case SSL_ERROR_WANT_WRITE:
                break;
            case SSL_ERROR_NONE:
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                onSuccess();
                break;
            default:
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                onError(sslErr);
                break;
        }
    }

    void TLSShutdown::readTimeout() {
        if (!timeoutTriggered) {
            timeoutTriggered = true;
            ReadEventReceiver::disable();
            WriteEventReceiver::disable();
            onTimeout();
        }
    }

    void TLSShutdown::writeTimeout() {
        if (!timeoutTriggered) {
            timeoutTriggered = true;
            ReadEventReceiver::disable();
            WriteEventReceiver::disable();
            onTimeout();
        }
    }

    void TLSShutdown::unobservedEvent() {
        delete this;
    }

    bool TLSShutdown::continueReadImmediately() const {
        return SSL_has_pending(ssl);
    }

} // namespace core::socket::stream::tls
