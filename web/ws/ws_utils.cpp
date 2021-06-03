/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    unsigned char* decode64(const char* input, int length) {
        const int pl = 3 * length / 4;
        unsigned char* output = reinterpret_cast<unsigned char*>(calloc(pl + 1, 1));
        const int ol = EVP_DecodeBlock(output, reinterpret_cast<const unsigned char*>(input), length);
        if (pl != ol) {
            LOG(ERROR) << "Whoops, decode predicted " << pl << " but we got " << ol;
        }
        return output;
    }

    char* base64(const unsigned char* input, int length) {
        const int pl = 4 * ((length + 2) / 3);
        char* output = reinterpret_cast<char*>(calloc(pl + 1, 1)); //+1 for the terminating null that EVP_EncodeBlock adds on
        const int ol = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(output), input, length);
        if (pl != ol) {
            LOG(ERROR) << "Whoops, encode predicted " << pl << " but we got " << ol;
        }
        return output;
    }

    void serverWebSocketKey(const std::string& clientWebSocketKey, const std::function<void(char*)>& returnKey) {
        std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

        std::string serverWebSocketKey(clientWebSocketKey + GUID);
        unsigned char digest[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(serverWebSocketKey.c_str()),
             serverWebSocketKey.length(),
             reinterpret_cast<unsigned char*>(&digest));

        char* key = base64(digest, SHA_DIGEST_LENGTH);
        returnKey(key);

        free(key);
    }

} // namespace web::ws

#include "ws_utils.h"
