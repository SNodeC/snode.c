#ifndef CORE_SOCKET_STREAM_TLS_DETAIL_TLSRESULT_H
#define CORE_SOCKET_STREAM_TLS_DETAIL_TLSRESULT_H

#include <cerrno>
#include <cstddef>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/types.h>
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

    struct TlsStatusInfo {
        TlsStatus status = TlsStatus::UnknownError;
        int sslError = SSL_ERROR_NONE;
        int systemError = 0;
        unsigned long openSslError = 0;
        int returnValue = 0;
    };

    struct TlsHandshakeSuccess {};

    struct TlsIoSuccess {
        ssize_t bytesTransferred = 0;
    };

    enum class TlsShutdownSuccess {
        CloseNotifySent,
        FullShutdownComplete
    };

    using TlsHandshakeResult = std::variant<TlsHandshakeSuccess, TlsStatusInfo>;
    using TlsIoResult = std::variant<TlsIoSuccess, TlsStatusInfo>;
    using TlsShutdownResult = std::variant<TlsShutdownSuccess, TlsStatusInfo>;

    inline bool isUnexpectedEofReason(unsigned long openSslError) {
#if defined(SSL_R_UNEXPECTED_EOF_WHILE_READING)
        return openSslError != 0 && ERR_GET_REASON(openSslError) == SSL_R_UNEXPECTED_EOF_WHILE_READING;
#else
        (void) openSslError;
        return false;
#endif
    }

    inline TlsStatusInfo classifyOpenSslFailure(int ret, int sslError, int savedErrno, unsigned long openSslError) {
        TlsStatus status = TlsStatus::UnknownError;
        switch (sslError) {
            case SSL_ERROR_WANT_READ:
                status = TlsStatus::WantRead;
                break;
            case SSL_ERROR_WANT_WRITE:
                status = TlsStatus::WantWrite;
                break;
            case SSL_ERROR_ZERO_RETURN:
                status = TlsStatus::CleanPeerShutdown;
                break;
            case SSL_ERROR_SYSCALL:
                if (savedErrno != 0) {
                    status = TlsStatus::SyscallError;
                } else if (ret == 0 && openSslError == 0) {
                    status = TlsStatus::UncleanEofWithoutCloseNotify;
                } else if (openSslError != 0) {
                    status = TlsStatus::SslProtocolError;
                } else {
                    status = TlsStatus::UnknownError;
                }
                break;
            case SSL_ERROR_SSL:
                status = isUnexpectedEofReason(openSslError) ? TlsStatus::UncleanEofWithoutCloseNotify : TlsStatus::SslProtocolError;
                break;
            default:
                status = TlsStatus::UnknownError;
                break;
        }
        return {status, sslError, savedErrno, openSslError, ret};
    }

    inline int fatalTlsStatusToErrno(const TlsStatusInfo& info) {
        switch (info.status) {
            case TlsStatus::UncleanEofWithoutCloseNotify:
            case TlsStatus::SslProtocolError:
                return EPROTO;
            case TlsStatus::SyscallError:
                return info.systemError == 0 ? EIO : info.systemError;
            case TlsStatus::UnknownError:
                return EIO;
            case TlsStatus::WantRead:
            case TlsStatus::WantWrite:
            case TlsStatus::CleanPeerShutdown:
                return 0;
        }
        return EIO;
    }

} // namespace core::socket::stream::tls::detail

#endif
