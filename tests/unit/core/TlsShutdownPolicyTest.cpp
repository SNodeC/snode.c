#include "core/socket/stream/tls/TLSShutdownPolicy.h"

#include "support/TestResult.h"

#include <cerrno>
#include <openssl/err.h>
#include <openssl/ssl.h>

using core::socket::stream::tls::TlsIoClassification;
using core::socket::stream::tls::TlsShutdownClassification;
using core::socket::stream::tls::classifyTlsIoResult;
using core::socket::stream::tls::classifyTlsShutdownResult;

namespace {
    unsigned long unexpectedEofErrorCode() {
#ifdef SSL_R_UNEXPECTED_EOF_WHILE_READING
        return ERR_PACK(ERR_LIB_SSL, 0, SSL_R_UNEXPECTED_EOF_WHILE_READING);
#else
        return 0;
#endif
    }
}

int main() {
    tests::support::TestResult result;

    result.expectTrue(classifyTlsIoResult(-1, SSL_ERROR_WANT_READ, 0, 0) == TlsIoClassification::WantRead,
                      "SSL_ERROR_WANT_READ waits for read readiness");
    result.expectTrue(classifyTlsIoResult(-1, SSL_ERROR_WANT_WRITE, 0, 0) == TlsIoClassification::WantWrite,
                      "SSL_ERROR_WANT_WRITE waits for write readiness");
    result.expectTrue(classifyTlsIoResult(0, SSL_ERROR_ZERO_RETURN, 0, 0) == TlsIoClassification::CleanCloseNotify,
                      "SSL_ERROR_ZERO_RETURN is clean close_notify");
    result.expectTrue(classifyTlsIoResult(0, SSL_ERROR_SYSCALL, 0, 0) == TlsIoClassification::UncleanEofWithoutCloseNotify,
                      "OpenSSL 1.1.1-style EOF without close_notify is unclean EOF");

#ifdef SSL_R_UNEXPECTED_EOF_WHILE_READING
    result.expectTrue(classifyTlsIoResult(-1, SSL_ERROR_SSL, 0, unexpectedEofErrorCode()) ==
                          TlsIoClassification::UncleanEofWithoutCloseNotify,
                      "OpenSSL 3.x-style unexpected EOF reason is unclean EOF");
#endif

    result.expectTrue(classifyTlsIoResult(-1, SSL_ERROR_SYSCALL, ECONNRESET, 0) == TlsIoClassification::SyscallError,
                      "errno-backed SSL_ERROR_SYSCALL is transport/syscall error");
    result.expectTrue(classifyTlsIoResult(-1, SSL_ERROR_SSL, 0, ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_RECORD_TYPE)) ==
                          TlsIoClassification::SslProtocolError,
                      "non-EOF SSL_ERROR_SSL remains TLS protocol/library error");

    result.expectTrue(classifyTlsShutdownResult(1, SSL_ERROR_NONE, 0, 0) == TlsShutdownClassification::FullShutdownComplete,
                      "SSL_shutdown return 1 is full bidirectional shutdown");
    result.expectTrue(classifyTlsShutdownResult(0, SSL_ERROR_NONE, 0, 0) == TlsShutdownClassification::CloseNotifySent,
                      "SSL_shutdown return 0 is close_notify sent, not an error");
    result.expectTrue(classifyTlsShutdownResult(-1, SSL_ERROR_WANT_READ, 0, 0) == TlsShutdownClassification::WantRead,
                      "SSL_shutdown WANT_READ retries readiness-driven");
    result.expectTrue(classifyTlsShutdownResult(-1, SSL_ERROR_WANT_WRITE, 0, 0) == TlsShutdownClassification::WantWrite,
                      "SSL_shutdown WANT_WRITE retries readiness-driven");
    result.expectTrue(classifyTlsShutdownResult(-1, SSL_ERROR_SYSCALL, ECONNRESET, 0) == TlsShutdownClassification::SyscallError,
                      "SSL_shutdown syscall failure is transport/syscall error");
    result.expectTrue(classifyTlsShutdownResult(-1, SSL_ERROR_SSL, 0, ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_RECORD_TYPE)) ==
                          TlsShutdownClassification::SslProtocolError,
                      "SSL_shutdown SSL_ERROR_SSL is TLS protocol/library error");

    return result.processResult();
}
