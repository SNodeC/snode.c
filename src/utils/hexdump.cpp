/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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
    std::string hexDump(const std::vector<char>& bytes, int prefixLength) {
        std::stringstream hexStream;

        if (!bytes.empty()) {
            uint8_t buff[17];
            size_t i = 0;

            hexStream << std::hex;

            int currentPrefixLength = 0;

            // Process every byte in the data.
            for (i = 0; i < bytes.size(); i++) {
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
                    currentPrefixLength = prefixLength;
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
