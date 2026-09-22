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

#include "hexdump.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    const HexDumpPalette plainHexDumpPalette{"", "", "", ""};
    const HexDumpPalette terminalHexDumpPalette{"\033[34m", "\033[32m", "\033[33m", "\033[39m"};

    std::string hexDump(const std::vector<char>& bytes, int prefixLength, bool prefixAtFirstLine) {
        return hexDump(bytes.data(), bytes.size(), prefixLength, prefixAtFirstLine);
    }

    std::string hexDump(const std::string& string, int prefixLength, bool prefixAtFirstLine) {
        return hexDump(string.data(), string.length(), prefixLength, prefixAtFirstLine);
    }

    std::string hexDump(const char* bytes, uint64_t length, int prefixLength, bool prefixAtFirstLine) {
        return hexDump(bytes, length, prefixLength, prefixAtFirstLine, plainHexDumpPalette);
    }

    std::string hexDump(const char* bytes, uint64_t length, int prefixLength, bool prefixAtFirstLine, const HexDumpPalette& palette) {
        constexpr char digits[] = "0123456789abcdef";
        std::string output;
        for (uint64_t offset = 0; offset < length;) {
            if (offset != 0)
                output += '\n';
            if (offset != 0 || prefixAtFirstLine)
                output.append(static_cast<std::size_t>(std::max(prefixLength, 0)), ' ');
            char address[16];
            const auto end = std::to_chars(address, address + sizeof(address), offset, 16).ptr;
            const auto addressLength = static_cast<std::size_t>(end - address);
            output += palette.offset;
            output.append(addressLength < 8 ? 8 - addressLength : 0, '0');
            output.append(address, addressLength);
            output += palette.reset;
            output += "  ";
            output += palette.byte;
            const auto count = std::min<uint64_t>(16, length - offset);
            char hex[48];
            char ascii[16];
            std::fill_n(hex, sizeof(hex), ' ');
            std::fill_n(ascii, sizeof(ascii), ' ');
            for (uint64_t column = 0; column < count; ++column) {
                const auto byte = static_cast<unsigned char>(bytes[offset + column]);
                const auto position = column * 3 + (column >= 8 ? 1 : 0);
                hex[position] = digits[byte >> 4];
                hex[position + 1] = digits[byte & 0x0f];
                ascii[column] = byte >= 0x20 && byte <= 0x7e ? static_cast<char>(byte) : '.';
            }
            output.append(hex, sizeof(hex));
            output += palette.reset;
            output += "  |";
            output += palette.ascii;
            output.append(ascii, sizeof(ascii));
            output += palette.reset;
            output += '|';
            offset += count;
        }
        return output;
    }

    HexDumpPresentation hexDumpPresentation(const std::vector<char>& bytes, int prefixLength, bool prefixAtFirstLine) {
        return hexDumpPresentation(bytes.data(), bytes.size(), prefixLength, prefixAtFirstLine);
    }

    HexDumpPresentation hexDumpPresentation(const std::string& string, int prefixLength, bool prefixAtFirstLine) {
        return hexDumpPresentation(string.data(), string.length(), prefixLength, prefixAtFirstLine);
    }

    HexDumpPresentation hexDumpPresentation(const char* bytes, uint64_t length, int prefixLength, bool prefixAtFirstLine) {
        return {hexDump(bytes, length, prefixLength, prefixAtFirstLine, plainHexDumpPalette),
                hexDump(bytes, length, prefixLength, prefixAtFirstLine, terminalHexDumpPalette)};
    }

} // namespace utils
