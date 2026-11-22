// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "net/stdio/stream/config/ConfigSocketEndpoint.h"
namespace net::stdio::stream::config {
    ConfigSocketEndpoint::ConfigSocketEndpoint(const std::string& name)
        : ConfigInstance(name, Role::ENDPOINT)
        , ConfigConnection(this)
        , Local(this)
        , Remote(this) {
    }
    ConfigSocketEndpoint::~ConfigSocketEndpoint() {
    }
} // namespace net::stdio::stream::config
