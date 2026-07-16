#ifndef CORE_SOCKET_STREAM_TLS_DETAIL_TLSHANDOFFTESTHOOKS_H
#define CORE_SOCKET_STREAM_TLS_DETAIL_TLSHANDOFFTESTHOOKS_H

#if defined(SNODEC_BUILD_TESTS)

#include <deque>
#include <optional>
#include <openssl/ssl.h>
#include <string>

namespace core::socket::stream::tls::detail::test {

    struct HandoffOperation { int returnValue = 1; };

    struct HandoffState {
        std::deque<int> sslPending;
        std::deque<HandoffOperation> sslReads;
        std::deque<std::size_t> bioPending;
        std::deque<HandoffOperation> bioReads;
        std::optional<int> sslHasPending;
        int sslReadBytes = 0;
        int bioReadBytes = 0;

        void reset() {
            sslPending.clear();
            sslReads.clear();
            bioPending.clear();
            bioReads.clear();
            sslHasPending.reset();
            sslReadBytes = 0;
            bioReadBytes = 0;
        }
    };

    HandoffState& handoffState();
    std::deque<std::string>& handoffPayloads();
    std::deque<std::string>& handoffBioPayloads();

} // namespace core::socket::stream::tls::detail::test

#endif

#endif
