/*
 * SNode.C - A Slim Toolkit for Network Communication
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef NET_IN_PHY_STREAM_PHYSICALSOCKETCLIENT_H
#define NET_IN_PHY_STREAM_PHYSICALSOCKETCLIENT_H

#include "net/in/phy/stream/PhysicalSocket.h" // IWYU pragma: export
#include "net/phy/stream/PhysicalSocketClient.h"

// IWYU pragma: no_include "net/in/phy/stream/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::phy::stream {

    class PhysicalSocketClient : public net::in::phy::stream::PhysicalSocket<net::phy::stream::PhysicalSocketClient> {
    public:
        using Super = net::in::phy::stream::PhysicalSocket<net::phy::stream::PhysicalSocketClient>;
        using Super::Super;

        PhysicalSocketClient(PhysicalSocketClient&&) noexcept = default;

        ~PhysicalSocketClient() override;
    };

} // namespace net::in::phy::stream

extern template class net::phy::stream::PhysicalSocketClient<net::in::SocketAddress>;
extern template class net::in::phy::stream::PhysicalSocket<net::phy::stream::PhysicalSocketClient>;
extern template class net::in::phy::PhysicalSocket<net::phy::stream::PhysicalSocketClient>;

#endif // NET_IN_PHY_STREAM_PHYSICALSOCKETCLIENT_H
