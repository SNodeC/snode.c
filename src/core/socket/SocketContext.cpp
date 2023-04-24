/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/socket/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    std::size_t SocketContext::sendToPeer(const std::string& data) const {
        return sendToPeer(data.data(), data.length());
    }

    void SocketContext::onWriteError(int errnum) {
        if (errnum != 0) {
            PLOG(ERROR) << "OnWriteError: " << errnum;
        }
    }

    void SocketContext::onReadError(int errnum) {
        if (errnum != 0) {
            PLOG(ERROR) << "OnReadError: " << errnum;
        }
    }

    void SocketContext::onExit() {
        LOG(INFO) << "Protocol exit";
    }

} // namespace core::socket
