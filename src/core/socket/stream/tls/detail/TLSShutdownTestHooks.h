#ifndef CORE_SOCKET_STREAM_TLS_DETAIL_TLSSHUTDOWNTESTHOOKS_H
#define CORE_SOCKET_STREAM_TLS_DETAIL_TLSSHUTDOWNTESTHOOKS_H

#include <cstddef>

namespace core::socket::stream::tls::detail::test {

    struct ForcedShutdownResult {
        int ret = 1;
        int sslError = 0;
        int systemError = 0;
        unsigned long openSslError = 0;
    };

    void resetTlsShutdownCounters();
    std::size_t tlsShutdownConstructedCount();
    std::size_t tlsShutdownActiveCount();
    std::size_t tlsShutdownMaxActiveCount();

    void forceNextTlsShutdownResult(const ForcedShutdownResult& result);
    void clearForcedTlsShutdownResult();

} // namespace core::socket::stream::tls::detail::test

#endif
