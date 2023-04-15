/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETCONNECTION_H
#define CORE_SOCKET_STREAM_TLS_SOCKETCONNECTION_H

#include "core/socket/stream/SocketConnection.h" // IWYU pragma: export
#include "core/socket/stream/tls/SocketReader.h" // IWYU pragma: export
#include "core/socket/stream/tls/SocketWriter.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <cstddef>

typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_st SSL;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename PhysicalSocketT>
    class SocketConnection final
        : public core::socket::stream::
              SocketConnectionT<PhysicalSocketT, core::socket::stream::tls::SocketReader, core::socket::stream::tls::SocketWriter> {
    private:
        using Super = core::socket::stream::
            SocketConnectionT<PhysicalSocketT, core::socket::stream::tls::SocketReader, core::socket::stream::tls::SocketWriter>;

        using PhysicalSocket = PhysicalSocketT;
        using SocketReader = typename Super::SocketReader;
        using SocketWriter = typename Super::SocketWriter;

    public:
        using SocketAddress = typename Super::SocketAddress;

        SocketConnection(int fd,
                         const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void(SocketConnection*)>& onDisconnect,
                         const utils::Timeval& readTimeout,
                         const utils::Timeval& writeTimeout,
                         std::size_t readBlockSize,
                         std::size_t writeBlockSize,
                         const utils::Timeval& terminateTimeout);

        SSL* getSSL() const;

    private:
        SSL* startSSL(SSL_CTX* ctx, const utils::Timeval& sslInitTimeout, const utils::Timeval& sslShutdownTimeout);

        void stopSSL();

        void doSSLHandshake(const std::function<void()>& onSuccess,
                            const std::function<void()>& onTimeout,
                            const std::function<void(int)>& onError) final;

        void doSSLShutdown(const std::function<void()>& onSuccess,
                           const std::function<void()>& onTimeout,
                           const std::function<void(int)>& onError,
                           const utils::Timeval& shutdownTimeout);

        using SocketWriter::doWriteShutdown;

        void doWriteShutdown() final;

        void doWriteShutdown(const std::function<void(int)>& onShutdown) final;

        SSL* ssl = nullptr;

        utils::Timeval sslInitTimeout;
        utils::Timeval sslShutdownTimeout;

        template <typename PhysicalSocket, typename Config>
        friend class SocketAcceptor;

        template <typename PhysicalSocket, typename Config>
        friend class SocketConnector;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETCONNECTION_H
