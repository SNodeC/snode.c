#ifndef CORE_SOCKET_STREAM_TLS_DETAIL_TLSRESULTS_H
#define CORE_SOCKET_STREAM_TLS_DETAIL_TLSRESULTS_H

#include <variant>

namespace core::socket::stream::tls::detail {

    enum class TlsStatus {
        WantRead,
        WantWrite,
        CleanPeerShutdown,
        UncleanEofWithoutCloseNotify,
        SyscallError,
        SslProtocolError,
        UnknownError
    };

    enum class TlsHandshakeSuccess { Complete };
    enum class TlsIoSuccess { DataTransferred };
    enum class TlsShutdownSuccess { CloseNotifySent, FullShutdownComplete };

    enum class TlsPostShutdownPolicy { CloseConnection, ContinuePlaintext };
    enum class TlsShutdownIntent { CloseConnection, ContinuePlaintext };
    enum class TlsTransportState { Handshaking, Encrypted, ShuttingDownForClose, ShuttingDownForPlaintext, Plaintext, Terminal };

    using TlsHandshakeResult = std::variant<TlsHandshakeSuccess, TlsStatus>;
    using TlsIoResult = std::variant<TlsIoSuccess, TlsStatus>;
    using TlsShutdownResult = std::variant<TlsShutdownSuccess, TlsStatus>;

} // namespace core::socket::stream::tls::detail

#endif
