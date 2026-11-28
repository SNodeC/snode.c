#include "core/socket/stream/tls/TLSHandshake.h"
#include "core/socket/stream/tls/TLSShutdown.h"

#include <functional>
#include <openssl/types.h>
#include <string>

namespace utils {
    class Timeval;
} // namespace utils

namespace {
    void compileSixArgumentCalls(const std::string& name,
                                 SSL* ssl,
                                 const std::function<void()>& onSuccess,
                                 const std::function<void()>& onTimeout,
                                 const std::function<void(int)>& onStatus,
                                 const utils::Timeval& timeout) {
        core::socket::stream::tls::TLSHandshake::doHandshake(name, ssl, onSuccess, onTimeout, onStatus, timeout);
        core::socket::stream::tls::TLSShutdown::doShutdown(name, ssl, onSuccess, onTimeout, onStatus, timeout);
    }
} // namespace

int main() {
    (void) &compileSixArgumentCalls;
    return 0;
}
