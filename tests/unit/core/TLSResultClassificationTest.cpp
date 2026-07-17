#include "core/socket/stream/tls/detail/TLSResult.h"
#include "support/TestResult.h"

#include <cerrno>
#include <openssl/err.h>
#include <openssl/ssl.h>

using core::socket::stream::tls::detail::classifyOpenSslFailure;
using core::socket::stream::tls::detail::fatalTlsStatusToErrno;
using core::socket::stream::tls::detail::TlsStatus;

namespace {

    unsigned long makeReason(int reason) {
        ERR_clear_error();
        ERR_put_error(ERR_LIB_SSL, 0, reason, __FILE__, __LINE__);
        const unsigned long err = ERR_peek_last_error();
        ERR_clear_error();
        return err;
    }

    void expectStatus(tests::support::TestResult& result,
                      const char* name,
                      int ret,
                      int sslError,
                      int systemError,
                      unsigned long openSslError,
                      TlsStatus expected,
                      int expectedFatalErrno) {
        const auto info = classifyOpenSslFailure(ret, sslError, systemError, openSslError);
        result.expectTrue(info.status == expected, std::string(name) + " status");
        result.expectEqual(info.sslError, sslError, std::string(name) + " sslError");
        result.expectEqual(info.systemError, systemError, std::string(name) + " systemError");
        result.expectEqual(info.openSslError, openSslError, std::string(name) + " openSslError");
        result.expectEqual(info.returnValue, ret, std::string(name) + " returnValue");
        if (expectedFatalErrno != -1) {
            result.expectEqual(fatalTlsStatusToErrno(info), expectedFatalErrno, std::string(name) + " fatal errno");
        }
    }

} // namespace

int main() {
    tests::support::TestResult result;

    expectStatus(result, "want read", -1, SSL_ERROR_WANT_READ, 0, 0, TlsStatus::WantRead, -1);
    expectStatus(result, "want write", -1, SSL_ERROR_WANT_WRITE, 0, 0, TlsStatus::WantWrite, -1);
    expectStatus(result, "zero return", 0, SSL_ERROR_ZERO_RETURN, 0, 0, TlsStatus::CleanPeerShutdown, -1);
    expectStatus(result, "syscall ECONNRESET", -1, SSL_ERROR_SYSCALL, ECONNRESET, 0, TlsStatus::SyscallError, ECONNRESET);
    expectStatus(result, "syscall EPIPE", -1, SSL_ERROR_SYSCALL, EPIPE, 0, TlsStatus::SyscallError, EPIPE);
    expectStatus(result, "syscall ETIMEDOUT", -1, SSL_ERROR_SYSCALL, ETIMEDOUT, 0, TlsStatus::SyscallError, ETIMEDOUT);
    expectStatus(result, "openssl 111 eof", 0, SSL_ERROR_SYSCALL, 0, 0, TlsStatus::UncleanEofWithoutCloseNotify, EPROTO);
    expectStatus(result,
                 "ambiguous syscall with queue",
                 -1,
                 SSL_ERROR_SYSCALL,
                 0,
                 makeReason(SSL_R_BAD_RECORD_TYPE),
                 TlsStatus::SslProtocolError,
                 EPROTO);
#if defined(SSL_R_UNEXPECTED_EOF_WHILE_READING)
    expectStatus(result,
                 "openssl 3 eof",
                 -1,
                 SSL_ERROR_SSL,
                 0,
                 makeReason(SSL_R_UNEXPECTED_EOF_WHILE_READING),
                 TlsStatus::UncleanEofWithoutCloseNotify,
                 EPROTO);
#endif
    expectStatus(result, "ssl protocol", -1, SSL_ERROR_SSL, 0, makeReason(SSL_R_BAD_RECORD_TYPE), TlsStatus::SslProtocolError, EPROTO);
    expectStatus(result, "unknown", -1, 12345, 0, 0, TlsStatus::UnknownError, EIO);

    return result.processResult();
}
