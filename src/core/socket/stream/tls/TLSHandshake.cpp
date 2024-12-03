/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include <openssl/ssl.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    void TLSHandshake::doHandshake(const std::string& instanceName,
                                   SSL* ssl,
                                   const std::function<void(void)>& onSuccess,
                                   const std::function<void(void)>& onTimeout,
                                   const std::function<void(int)>& onStatus,
                                   const utils::Timeval& timeout) {
        new TLSHandshake(instanceName, ssl, onSuccess, onTimeout, onStatus, timeout);
    }

    TLSHandshake::TLSHandshake(const std::string& instanceName,
                               SSL* ssl,
                               const std::function<void(void)>& onSuccess,
                               const std::function<void(void)>& onTimeout,
                               const std::function<void(int)>& onStatus,
                               const utils::Timeval& timeout)
        : ReadEventReceiver(instanceName + " SSL/TLS: Handshake", timeout)
        , WriteEventReceiver(instanceName + " SSL/TLS: Handshake", timeout)
        , ssl(ssl)
        , onSuccess(onSuccess)
        , onTimeout(onTimeout)
        , onStatus(onStatus)
        , timeoutTriggered(false)
        , fd(SSL_get_fd(ssl)) {
        const int ret = SSL_do_handshake(ssl);

        int sslErr = SSL_ERROR_NONE;
        if (ret < 1) {
            sslErr = SSL_get_error(ssl, ret);
        }

        if (!ReadEventReceiver::enable(fd)) {
            delete this;
        } else if (!WriteEventReceiver::enable(fd)) {
            ReadEventReceiver::disable();
        } else {
            ReadEventReceiver::suspend();
            WriteEventReceiver::suspend();

            switch (sslErr) {
                case SSL_ERROR_WANT_READ:
                    ReadEventReceiver::resume();
                    break;
                case SSL_ERROR_WANT_WRITE:
                    WriteEventReceiver::resume();
                    break;
                case SSL_ERROR_NONE:
                case SSL_ERROR_ZERO_RETURN:
                    onSuccess();
                    ReadEventReceiver::disable();
                    WriteEventReceiver::disable();
                    break;
                default:
                    onStatus(sslErr);
                    ReadEventReceiver::disable();
                    WriteEventReceiver::disable();
                    break;
            }
        }
    }

    void TLSHandshake::readEvent() {
        const int ret = SSL_do_handshake(ssl);

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
            case SSL_ERROR_ZERO_RETURN:
                onSuccess();
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                break;
            default:
                onStatus(sslErr);
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                break;
        }
    }

    void TLSHandshake::writeEvent() {
        const int ret = SSL_do_handshake(ssl);

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
            case SSL_ERROR_ZERO_RETURN:
                onSuccess();
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                break;
            default:
                onStatus(sslErr);
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

    void TLSHandshake::signalEvent([[maybe_unused]] int signum) { // Do nothing on signal event
    }

    void TLSHandshake::unobservedEvent() {
        delete this;
    }

} // namespace core::socket::stream::tls
