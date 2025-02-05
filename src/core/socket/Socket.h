/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef CORE_SOCKET_LOGICALSOCKET_H
#define CORE_SOCKET_LOGICALSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory> // IWYU pragma: export
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    template <typename ConfigT>
    class Socket {
    public:
        using Config = ConfigT;

        explicit Socket(const std::string& name);

        Socket(const Socket&) = default;
        Socket& operator=(const Socket&) = default;

        Socket(Socket&&) noexcept = delete;
        Socket& operator=(Socket&&) noexcept = delete;

        virtual ~Socket();

        Config& getConfig() const;

    protected:
        std::shared_ptr<Config> config;
    };

} // namespace core::socket

#endif // CORE_SOCKET_LOGICALSOCKET_H
