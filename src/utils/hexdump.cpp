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

#include "hexdump.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    // From: https://gist.github.com/shreyasbharath/32a8092666303a916e24a81b18af146b
    std::string hexDump(const std::vector<char>& bytes, int prefixLength, bool prefixAtFirstLine) {
        return hexDump(bytes.data(), bytes.size(), prefixLength, prefixAtFirstLine);
    }

    std::string hexDump(const std::string& string, int prefixLength, bool prefixAtFirstLine) {
        return hexDump(string.data(), string.length(), prefixLength, prefixAtFirstLine);
    }

    std::string hexDump(const char* bytes, uint64_t length, int prefixLength, bool prefixAtFirstLine) {
        std::stringstream hexStream;

        if (length > 0) {
            uint8_t buff[17];
            size_t i = 0;

            hexStream << std::hex;

            int currentPrefixLength = prefixAtFirstLine ? prefixLength : 0;

            // Process every byte in the data.
            for (i = 0; i < length; i++) {
                // Multiple of 16 means new line (with line offset).

                if ((i % 16) == 0) {
                    // Just don't print ASCII for the zeroth line.
                    if (i != 0) {
                        hexStream << "  " << buff << std::endl;
                    }

                    // Output the offset.
                    hexStream << Color::Code::FG_BLUE;
                    hexStream << std::setw(currentPrefixLength) << std::setfill(' ') << ""
                              << ": " << std::setw(8) << std::setfill('0') << static_cast<unsigned int>(i);
                    hexStream << Color::Code::FG_DEFAULT << " ";
                }

                // Now the hex code for the specific character.
                hexStream << Color::Code::FG_GREEN;
                hexStream << " " << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(bytes[i]));
                hexStream << Color::Code::FG_DEFAULT;

                // And store a printable ASCII character for later.
                if ((bytes[i] < 0x20) || (bytes[i] > 0x7e)) {
                    buff[i % 16] = '.';
                } else {
                    buff[i % 16] = static_cast<uint8_t>(bytes[i]);
                }
                buff[(i % 16) + 1] = '\0';

                currentPrefixLength = prefixLength;
            }

            hexStream << std::dec;

            // Pad out last line if not exactly 16 characters.
            while ((i % 16) != 0) {
                hexStream << "   ";
                i++;
            }

            // And print the final ASCII bit.
            hexStream << "  " << buff;
        }

        return hexStream.str();
    }

} // namespace utils
