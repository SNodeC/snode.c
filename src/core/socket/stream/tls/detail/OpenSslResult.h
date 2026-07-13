#ifndef CORE_SOCKET_STREAM_TLS_DETAIL_OPENSSLRESULT_H
#define CORE_SOCKET_STREAM_TLS_DETAIL_OPENSSLRESULT_H

#include "core/socket/stream/tls/detail/TlsResults.h"

#include <cerrno>
#include <openssl/err.h>
#include <openssl/ssl.h>

#ifndef SSL_R_UNEXPECTED_EOF_WHILE_READING
#define SSL_R_UNEXPECTED_EOF_WHILE_READING 294
#endif

namespace core::socket::stream::tls::detail {

    struct OpenSslFailure {
        int sslError = SSL_ERROR_NONE;
        int systemError = 0;
        unsigned long openSslError = 0;
    };

    inline int mappedSystemError(TlsStatus status, int savedErrno) {
        switch (status) {
            case TlsStatus::SyscallError:
                return savedErrno == 0 ? EIO : savedErrno;
            case TlsStatus::UncleanEofWithoutCloseNotify:
            case TlsStatus::SslProtocolError:
            case TlsStatus::CleanPeerShutdown:
                return EPROTO;
            case TlsStatus::UnknownError:
                return EIO;
            case TlsStatus::WantRead:
            case TlsStatus::WantWrite:
                return 0;
        }
        return EIO;
    }

    inline TlsStatus classifyOpenSslFailure(int ret, const OpenSslFailure& failure) {
        switch (failure.sslError) {
            case SSL_ERROR_WANT_READ:
                return TlsStatus::WantRead;
            case SSL_ERROR_WANT_WRITE:
                return TlsStatus::WantWrite;
            case SSL_ERROR_ZERO_RETURN:
                return TlsStatus::CleanPeerShutdown;
            case SSL_ERROR_SYSCALL:
                if (failure.systemError != 0) {
                    return TlsStatus::SyscallError;
                }
                if (ret == 0 && failure.openSslError == 0) {
                    return TlsStatus::UncleanEofWithoutCloseNotify;
                }
                return TlsStatus::UnknownError;
            case SSL_ERROR_SSL:
                if (failure.openSslError != 0 && ERR_GET_REASON(failure.openSslError) == SSL_R_UNEXPECTED_EOF_WHILE_READING) {
                    return TlsStatus::UncleanEofWithoutCloseNotify;
                }
                return TlsStatus::SslProtocolError;
            default:
                return TlsStatus::UnknownError;
        }
    }

} // namespace core::socket::stream::tls::detail

#endif
