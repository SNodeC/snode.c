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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETREADER_H
#define CORE_SOCKET_STREAM_TLS_SOCKETREADER_H

#include "core/socket/stream/SocketReader.h"
#include "core/socket/stream/tls/TLSHandshake.h"
#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for std::size_t
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketT>
    class SocketReader : public core::socket::stream::SocketReader<SocketT> {
        using core::socket::stream::SocketReader<SocketT>::SocketReader;

    private:
        ssize_t read(char* junk, std::size_t junkLen) override {
            sslErr = 0;

            int ret = SSL_read(ssl, junk, static_cast<int>(junkLen));

            if (ret <= 0) {
                sslErr = SSL_get_error(ssl, ret);

                switch (sslErr) {
                    case SSL_ERROR_WANT_WRITE:
                        LOG(INFO) << "SSL/TLS start renegotiation on read";
                        doSSLHandshake(
                            [](void) -> void {
                                LOG(INFO) << "SSL/TLS renegotiation on read success";
                            },
                            [](void) -> void {
                                LOG(WARNING) << "SSL/TLS renegotiation on read timed out";
                            },
                            [this](int sslErr) -> void {
                                ssl_log("SSL/TLS renegotiation", sslErr);
                                this->sslErr = sslErr;
                            });
                        errno = EAGAIN;
                        break;
                    case SSL_ERROR_WANT_READ:
                        ret = 0;
                        errno = EAGAIN;
                        break;
                    case SSL_ERROR_ZERO_RETURN: // shutdonw cleanly
                        ret = 0;                // On the read side propagate the zerro
                        break;
                    case SSL_ERROR_SYSCALL:
                        ret = -1;
                        break;
                    default:
                        ssl_log("SSL/TLS read failed", sslErr);
                        ret = -1;
                        break;
                }
            }

            return ret;
        }

        bool continueReadImmediately() override {
            return SSL_pending(ssl) || core::socket::stream::SocketReader<SocketT>::continueReadImmediately();
        }

        virtual void readEvent() override = 0;

    protected:
        virtual void doSSLHandshake(const std::function<void()>& onSuccess,
                                    const std::function<void()>& onTimeout,
                                    const std::function<void(int)>& onError) = 0;

        SSL* ssl = nullptr;

        int sslErr = SSL_ERROR_NONE;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETREADER_H
