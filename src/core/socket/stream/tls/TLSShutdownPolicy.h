/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#ifndef CORE_SOCKET_STREAM_TLS_TLSSHUTDOWNPOLICY_H
#define CORE_SOCKET_STREAM_TLS_TLSSHUTDOWNPOLICY_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/err.h>
#include <openssl/ssl.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    enum class TlsIoClassification {
        Success,
        WantRead,
        WantWrite,
        CleanCloseNotify,
        UncleanEofWithoutCloseNotify,
        SyscallError,
        SslProtocolError,
        UnknownError
    };

    enum class TlsShutdownClassification {
        FullShutdownComplete,
        CloseNotifySent,
        WantRead,
        WantWrite,
        SyscallError,
        SslProtocolError,
        UnknownError
    };

    TlsIoClassification classifyTlsIoResult(int ret, int sslErr, int savedErrno, unsigned long peekedOpenSslError);
    TlsShutdownClassification classifyTlsShutdownResult(int ret, int sslErr, int savedErrno, unsigned long peekedOpenSslError);

    bool isOpenSslUnexpectedEof(unsigned long openSslError);

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_TLSSHUTDOWNPOLICY_H
