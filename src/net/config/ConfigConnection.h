/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#ifndef NET_CONFIG_CONFIGCONN_H
#define NET_CONFIG_CONFIGCONN_H

#include "net/config/ConfigSection.h"

namespace net::config {
    class ConfigInstance;
}
#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class Option;
} // namespace CLI

#include "utils/Timeval.h"

#include <cstddef>
#include <string_view>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    class ConfigConnection : protected ConfigSection {
    public:
        using Connection = ConfigConnection;

    protected:
        explicit ConfigConnection(ConfigInstance* instance);

        ~ConfigConnection() override;

    public:
        constexpr static std::string_view name{"connection"};

        utils::Timeval getReadTimeout() const;
        ConfigConnection& setReadTimeout(const utils::Timeval& newReadTimeoutSet);

        utils::Timeval getWriteTimeout() const;
        ConfigConnection& setWriteTimeout(const utils::Timeval& newWriteTimeoutSet);

        std::size_t getReadBlockSize() const;
        ConfigConnection& setReadBlockSize(std::size_t newReadBlockSize);

        std::size_t getWriteBlockSize() const;
        ConfigConnection& setWriteBlockSize(std::size_t newWriteBlockSize);

        utils::Timeval getTerminateTimeout() const;
        ConfigConnection& setTerminateTimeout(const utils::Timeval& newTerminateTimeout);

    private:
        CLI::Option* readTimeoutOpt = nullptr;
        CLI::Option* writeTimeoutOpt = nullptr;
        CLI::Option* readBlockSizeOpt = nullptr;
        CLI::Option* writeBlockSizeOpt = nullptr;
        CLI::Option* terminateTimeoutOpt = nullptr;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGCONN_H
