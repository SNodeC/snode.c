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

#include "mcp/detail/Utilities.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <array>
#include <cerrno>
#include <ctime>
#include <stdexcept>
#include <string>
#include <sys/random.h>
#endif
namespace mcp::detail {
    std::string randomToken() {
        std::array<unsigned char, 32> bytes{};
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            const auto count = ::getrandom(bytes.data() + offset, bytes.size() - offset, 0);
            if (count > 0) {
                offset += static_cast<std::size_t>(count);
            } else if (count == 0 || errno != EINTR) {
                throw std::runtime_error("Secure random source unavailable");
            }
        }
        std::string value;
        for (const auto byte : bytes) {
            value += "0123456789abcdef"[byte >> 4U];
            value += "0123456789abcdef"[byte & 15U];
        }
        return value;
    }

    std::string timestamp() {
        const auto now = std::time(nullptr);
        std::tm utc{};
        std::array<char, 32> text{};
        if (::gmtime_r(&now, &utc) == nullptr || std::strftime(text.data(), text.size(), "%Y-%m-%dT%H:%M:%SZ", &utc) == 0) {
            throw std::runtime_error("UTC timestamp unavailable");
        }
        return text.data();
    }
} // namespace mcp::detail
