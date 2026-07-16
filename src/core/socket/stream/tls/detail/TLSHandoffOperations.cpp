#include "core/socket/stream/tls/TLSHandoffOperations.h"

#if defined(SNODEC_BUILD_TESTS)
#include "core/socket/stream/tls/detail/TLSHandoffTestHooks.h"
#endif

#include <algorithm>
#include <string>

namespace core::socket::stream::tls::detail {

    int tlsHandoffSslPending(SSL* ssl) {
#if defined(SNODEC_BUILD_TESTS)
        auto& state = test::handoffState();
        if (!state.sslPending.empty()) {
            const int pending = state.sslPending.front();
            state.sslPending.pop_front();
            return pending;
        }
#endif
        return SSL_pending(ssl);
    }

    int tlsHandoffSslRead(SSL* ssl, char* buffer, std::size_t size) {
#if defined(SNODEC_BUILD_TESTS)
        auto& state = test::handoffState();
        if (!state.sslReads.empty()) {
            const auto result = state.sslReads.front();
            state.sslReads.pop_front();
            if (result.returnValue > 0) {
                const std::string payload = test::handoffPayloads().front();
                test::handoffPayloads().pop_front();
                std::copy(payload.begin(), payload.begin() + std::min<std::size_t>(payload.size(), size), buffer);
                state.sslReadBytes += result.returnValue;
            }
            return result.returnValue;
        }
#endif
        return SSL_read(ssl, buffer, static_cast<int>(size));
    }

    long tlsHandoffBioPending(BIO* bio) {
#if defined(SNODEC_BUILD_TESTS)
        auto& state = test::handoffState();
        if (!state.bioPending.empty()) {
            const long pending = state.bioPending.front();
            state.bioPending.pop_front();
            return pending;
        }
#endif
        return bio == nullptr ? 0 : BIO_ctrl_pending(bio);
    }

    int tlsHandoffBioRead(BIO* bio, char* buffer, std::size_t size) {
#if defined(SNODEC_BUILD_TESTS)
        auto& state = test::handoffState();
        if (!state.bioReads.empty()) {
            const auto result = state.bioReads.front();
            state.bioReads.pop_front();
            if (result.returnValue > 0) {
                const std::string payload = test::handoffBioPayloads().front();
                test::handoffBioPayloads().pop_front();
                std::copy(payload.begin(), payload.begin() + std::min<std::size_t>(payload.size(), size), buffer);
                state.bioReadBytes += result.returnValue;
            }
            return result.returnValue;
        }
#endif
        return BIO_read(bio, buffer, static_cast<int>(size));
    }

    int tlsHandoffSslHasPending(SSL* ssl) {
#if defined(SNODEC_BUILD_TESTS)
        auto& state = test::handoffState();
        if (state.sslHasPending.has_value()) {
            return *state.sslHasPending;
        }
#endif
        return SSL_has_pending(ssl);
    }

} // namespace core::socket::stream::tls::detail
