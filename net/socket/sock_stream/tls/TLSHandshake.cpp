/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/err.h>
#include <openssl/ssl.h> // IWYU pragma: keep

// IWYU pragma: no_include <openssl/ssl3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "TLSHandshake.h"

namespace net::socket::stream::tls {

    TLSHandshake::TLSHandshake(SSL* ssl,
                               const std::function<void(void)>& onSuccess,
                               const std::function<void(void)>& onTimeout,
                               const std::function<void(int err)>& onError)
        : ssl(ssl)
        , onSuccess(onSuccess)
        , onTimeout(onTimeout)
        , onError(onError) {
        ReadEventReceiver::setTimeout(TLSHANDSHAKE_TIMEOUT);
        WriteEventReceiver::setTimeout(TLSHANDSHAKE_TIMEOUT);

        open(SSL_get_fd(ssl), FLAGS::dontClose);

        int ret = SSL_do_handshake(ssl);
        int sslErr = SSL_get_error(ssl, ret);

        switch (sslErr) {
            case SSL_ERROR_WANT_READ:
                ReadEventReceiver::enable();
                break;
            case SSL_ERROR_WANT_WRITE:
                WriteEventReceiver::enable();
                break;
            case SSL_ERROR_NONE:
                onSuccess();
                delete this;
                break;
            default:
                errorEvent(sslErr);
                delete this;
        }
    }

    void TLSHandshake::readEvent() {
        int ret = SSL_do_handshake(ssl);
        int sslErr = SSL_get_error(ssl, ret);

        switch (sslErr) {
            case SSL_ERROR_WANT_READ:
                break;
            case SSL_ERROR_WANT_WRITE:
                ReadEventReceiver::disable();
                WriteEventReceiver::enable();
                break;
            case SSL_ERROR_NONE:
                ReadEventReceiver::disable();
                onSuccess();
                break;
            default:
                ReadEventReceiver::disable();
                errorEvent(sslErr);
                break;
        }
    }

    void TLSHandshake::writeEvent() {
        int ret = SSL_do_handshake(ssl);
        int sslErr = SSL_get_error(ssl, ret);

        switch (sslErr) {
            case SSL_ERROR_WANT_READ:
                WriteEventReceiver::disable();
                ReadEventReceiver::enable();
                break;
            case SSL_ERROR_WANT_WRITE:
                break;
            case SSL_ERROR_NONE:
                ReadEventReceiver::disable();
                onSuccess();
                break;
            default:
                WriteEventReceiver::disable();
                errorEvent(sslErr);
                break;
        }
    }

    void TLSHandshake::timeoutEvent() {
        PLOG(ERROR) << "SSL/TLS handshake timeout";

        ReadEventReceiver::suspend();
        WriteEventReceiver::suspend();
        onTimeout();
    }

    void TLSHandshake::errorEvent(int sslErr) {
        PLOG(ERROR) << "SSL/TLS handshake failed: " << ERR_error_string(sslErr, nullptr);
        onError(-ERR_get_error());
    }

    void TLSHandshake::unobserved() {
        delete this;
    }

    void TLSHandshake::doHandshake(SSL* ssl,
                                   const std::function<void(void)>& onSuccess,
                                   const std::function<void(void)>& onTimeout,
                                   const std::function<void(int err)>& onError) {
        new TLSHandshake(ssl, onSuccess, onTimeout, onError);
    }

} // namespace net::socket::stream::tls
