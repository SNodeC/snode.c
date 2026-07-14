#include "core/socket/stream/tls/TLSHandshake.h"
#include "core/socket/stream/tls/TLSShutdown.h"
#include "utils/Timeval.h"

#include <functional>
#include <openssl/ssl.h>
#include <string>

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
}

int main() {
    (void)&compileSixArgumentCalls;
    return 0;
}
