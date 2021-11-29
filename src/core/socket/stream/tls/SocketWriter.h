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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETWRITER_H
#define CORE_SOCKET_STREAM_TLS_SOCKETWRITER_H

#include "core/socket/stream/SocketWriter.h"
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
    class SocketWriter : public core::socket::stream::SocketWriter<SocketT> {
        using core::socket::stream::SocketWriter<SocketT>::SocketWriter;

    private:
        ssize_t write(const char* junk, std::size_t junkLen) override {
            sslErr = 0;

            int ret = SSL_write(ssl, junk, static_cast<int>(junkLen));

            if (ret <= 0) {
                sslErr = SSL_get_error(ssl, ret);

                switch (sslErr) {
                    case SSL_ERROR_WANT_READ:
                        LOG(INFO) << "SSL/TLS start renegotiation on write";
                        doSSLHandshake(
                            [](void) -> void {
                                LOG(INFO) << "SSL/TLS renegotiation on write success";
                            },
                            [](void) -> void {
                                LOG(WARNING) << "SSL/TLS renegotiation on write timed out";
                            },
                            [this](int sslErr) -> void {
                                ssl_log("SSL/TLS renegotiation", sslErr);
                                setSSLError(sslErr);
                            });
                        errno = EAGAIN;
                        break;
                    case SSL_ERROR_WANT_WRITE:
                        ret = 0;
                        errno = EAGAIN;
                        break;
                    case SSL_ERROR_ZERO_RETURN: // shutdown cleanly
                        ret = -1;               // on the write side this means a TCP broken pipe
                        break;
                    case SSL_ERROR_SYSCALL:
                        ret = -1;
                        break;
                    default:
                        ssl_log("SSL/TLS write failed", sslErr);
                        ret = -1;
                        break;
                }
            }

            return ret;
        }

        int getError() override {
            int ret = errno;

            switch (sslErr) {
                case SSL_ERROR_NONE:
                    break;
                case SSL_ERROR_ZERO_RETURN:
                    break;
                case SSL_ERROR_SYSCALL:
                    break;
                default:
                    ret = errno;
                    break;
            };

            return ret;
        }

        virtual void writeEvent() override = 0;

    protected:
        void terminate() override {
            core::socket::stream::SocketWriter<SocketT>::terminate();
        }

        virtual void doSSLHandshake(const std::function<void()>& onSuccess,
                                    const std::function<void()>& onTimeout,
                                    const std::function<void(int)>& onError) = 0;

        void setSSLError(int sslErr) {
            this->sslErr = sslErr;
        }

        SSL* ssl = nullptr;

        int sslErr = SSL_ERROR_NONE;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETWRITER_H
