/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "core/socket/stream/tls/TLSShutdownPolicy.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    bool isOpenSslUnexpectedEof(unsigned long openSslError) {
#ifdef SSL_R_UNEXPECTED_EOF_WHILE_READING
        return openSslError != 0 && ERR_GET_REASON(openSslError) == SSL_R_UNEXPECTED_EOF_WHILE_READING;
#else
        (void) openSslError;
        return false;
#endif
    }

    TlsIoClassification classifyTlsIoResult(int ret, int sslErr, int savedErrno, unsigned long peekedOpenSslError) {
        if (ret > 0 || sslErr == SSL_ERROR_NONE) {
            return TlsIoClassification::Success;
        }

        switch (sslErr) {
            case SSL_ERROR_WANT_READ:
                return TlsIoClassification::WantRead;
            case SSL_ERROR_WANT_WRITE:
                return TlsIoClassification::WantWrite;
            case SSL_ERROR_ZERO_RETURN:
                return TlsIoClassification::CleanCloseNotify;
            case SSL_ERROR_SYSCALL:
                // OpenSSL 1.1.1 reports EOF without TLS close_notify as syscall, ret == 0, errno == 0, empty queue.
                if (ret == 0 && savedErrno == 0 && peekedOpenSslError == 0) {
                    return TlsIoClassification::UncleanEofWithoutCloseNotify;
                }
                return TlsIoClassification::SyscallError;
            case SSL_ERROR_SSL:
                // OpenSSL 3.x reports EOF without TLS close_notify as SSL_R_UNEXPECTED_EOF_WHILE_READING.
                if (isOpenSslUnexpectedEof(peekedOpenSslError)) {
                    return TlsIoClassification::UncleanEofWithoutCloseNotify;
                }
                return TlsIoClassification::SslProtocolError;
            default:
                return TlsIoClassification::UnknownError;
        }
    }

    TlsShutdownClassification classifyTlsShutdownResult(int ret, int sslErr, int savedErrno, unsigned long peekedOpenSslError) {
        (void) savedErrno;
        (void) peekedOpenSslError;

        if (ret == 1) {
            return TlsShutdownClassification::FullShutdownComplete;
        }
        if (ret == 0) {
            // SSL_shutdown() returning 0 means local close_notify was sent but peer close_notify was not yet received.
            return TlsShutdownClassification::CloseNotifySent;
        }

        switch (sslErr) {
            case SSL_ERROR_WANT_READ:
                return TlsShutdownClassification::WantRead;
            case SSL_ERROR_WANT_WRITE:
                return TlsShutdownClassification::WantWrite;
            case SSL_ERROR_SYSCALL:
                return TlsShutdownClassification::SyscallError;
            case SSL_ERROR_SSL:
                return TlsShutdownClassification::SslProtocolError;
            default:
                return TlsShutdownClassification::UnknownError;
        }
    }

} // namespace core::socket::stream::tls
