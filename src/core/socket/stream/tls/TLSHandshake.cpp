/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "core/socket/stream/tls/TLSHandshake.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/ssl.h> // IWYU pragma: keep

// IWYU pragma: no_include <openssl/ssl3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    void TLSHandshake::doHandshake(SSL* ssl,
                                   const std::function<void(void)>& onSuccess,
                                   const std::function<void(void)>& onTimeout,
                                   const std::function<void(int err)>& onError,
                                   const utils::Timeval& timeout) {
        new TLSHandshake(ssl, onSuccess, onTimeout, onError, timeout);
    }

    TLSHandshake::TLSHandshake(SSL* ssl,
                               const std::function<void(void)>& onSuccess,
                               const std::function<void(void)>& onTimeout,
                               const std::function<void(int err)>& onError,
                               const utils::Timeval& timeout)
        : ReadEventReceiver(timeout)
        , WriteEventReceiver(timeout)
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

        int ret = SSL_do_handshake(ssl);

        int sslErr = SSL_ERROR_NONE;
        if (ret < 1) {
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
                onSuccess();
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                break;
            default:
                onError(sslErr);
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                break;
        }
    }

    void TLSHandshake::readEvent() {
        int ret = SSL_do_handshake(ssl);

        int sslErr = SSL_ERROR_NONE;
        if (ret < 1) {
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
                onSuccess();
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                break;
            default:
                onError(sslErr);
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                break;
        }
    }

    void TLSHandshake::writeEvent() {
        int ret = SSL_do_handshake(ssl);

        int sslErr = SSL_ERROR_NONE;
        if (ret < 1) {
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
                onSuccess();
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                break;
            default:
                onError(sslErr);
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                break;
        }
    }

    void TLSHandshake::readTimeout() {
        if (!timeoutTriggered) {
            timeoutTriggered = true;
            onTimeout();
            ReadEventReceiver::disable();
            WriteEventReceiver::disable();
        }
    }

    void TLSHandshake::writeTimeout() {
        if (!timeoutTriggered) {
            timeoutTriggered = true;
            onTimeout();
            ReadEventReceiver::disable();
            WriteEventReceiver::disable();
        }
    }

    void TLSHandshake::unobservedEvent() {
        delete this;
    }

} // namespace core::socket::stream::tls
