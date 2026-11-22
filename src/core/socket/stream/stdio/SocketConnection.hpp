// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/socket/stream/SocketConnection.hpp"
#include "core/socket/stream/stdio/SocketConnection.h"

#include <utility>

namespace core::socket::stream::stdio {

    template <typename PhysicalSocket, typename Config>
    SocketConnection<PhysicalSocket, Config>::SocketConnection(PhysicalSocket&& physicalSocket,
                                                               const std::function<void(SocketConnection*)>& onDisconnect,
                                                               std::uint64_t connectionId,
                                                               const std::shared_ptr<Config>& config)
        : Super(
              std::move(physicalSocket),
              [onDisconnect, this]() {
                  onDisconnect(this);
              },
              connectionId,
              config) {
    }

} // namespace core::socket::stream::stdio
