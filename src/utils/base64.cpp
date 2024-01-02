/*
 * SNode.C - a slim toolkit for network communication
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/base64.h"

#include "utils/sha1.h"

#include <cctype>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace base64 {

    std::string serverWebSocketKey(const std::string& clientWebSocketKey) {
        const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

        const std::string serverWebSocketKey(clientWebSocketKey + GUID);
        std::vector<unsigned char> digest = sha1(serverWebSocketKey);

        return base64_encode(digest.data(), digest.size());
    }

    static bool is_base64(char c) {
        return ((isalnum(c) != 0) || (c == '+') || (c == '/'));
    }

    std::string base64_encode(const unsigned char* bytes_to_encode, std::size_t length) {
        static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                "abcdefghijklmnopqrstuvwxyz"
                                                "0123456789+/";
        std::string ret;
        int i = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (length-- > 0) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = static_cast<unsigned char>(((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
                char_array_4[2] = static_cast<unsigned char>(((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; i < 4; i++) {
                    ret += base64_chars[char_array_4[i]];
                }
                i = 0;
            }
        }

        if (i != 0) {
            for (int j = i; j < 3; j++) {
                char_array_3[j] = '\0';
            }

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = static_cast<unsigned char>(((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
            char_array_4[2] = static_cast<unsigned char>(((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (int j = 0; j < i + 1; j++) {
                ret += base64_chars[char_array_4[j]];
            }

            while (i++ < 3) {
                ret += '=';
            }
        }

        return ret;
    }

    std::string base64_decode(const std::string& encoded_string) {
        static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                "abcdefghijklmnopqrstuvwxyz"
                                                "0123456789+/";
        std::string::size_type in_len = encoded_string.size();
        std::string::size_type i = 0;
        std::string::size_type in_ = 0;
        char char_array_4[4];
        char char_array_3[3];
        std::string ret;

        while (in_len-- > 0 && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
            char_array_4[i++] = encoded_string[in_];
            in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++) {
                    char_array_4[i] = static_cast<char>(base64_chars.find(char_array_4[i]));
                }

                char_array_3[0] = static_cast<char>((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4));
                char_array_3[1] = static_cast<char>(((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2));
                char_array_3[2] = static_cast<char>(((char_array_4[2] & 0x3) << 6) + char_array_4[3]);

                for (i = 0; (i < 3); i++) {
                    ret += char_array_3[i];
                }
                i = 0;
            }
        }

        if (i != 0) {
            for (std::string::size_type j = i; j < 4; j++) {
                char_array_4[j] = 0;
            }

            for (std::string::size_type j = 0; j < 4; j++) {
                char_array_4[j] = static_cast<char>(base64_chars.find(char_array_4[j]));
            }

            char_array_3[0] = static_cast<char>((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4));
            char_array_3[1] = static_cast<char>(((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2));
            char_array_3[2] = static_cast<char>(((char_array_4[2] & 0x3) << 6) + char_array_4[3]);

            for (std::string::size_type j = 0; j < i - 1; j++) {
                ret += char_array_3[j];
            }
        }

        return ret;
    }
} // namespace base64
