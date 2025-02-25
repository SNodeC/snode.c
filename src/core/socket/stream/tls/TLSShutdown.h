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

#ifndef CORE_SOCKET_STREAM_TLS_TLSSHUTDOWN_H
#define CORE_SOCKET_STREAM_TLS_TLSSHUTDOWN_H

#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/eventreceiver/WriteEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace utils {
    class Timeval;
}

#include <functional>
#include <openssl/opensslv.h>
#include <string>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/types.h>
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
#include <openssl/ossl_typ.h>
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    class TLSShutdown
        : public core::eventreceiver::ReadEventReceiver
        , public core::eventreceiver::WriteEventReceiver {
    public:
        static void doShutdown(const std::string& instanceName,
                               SSL* ssl,
                               const std::function<void(void)>& onSuccess,
                               const std::function<void(void)>& onTimeout,
                               const std::function<void(int)>& onStatus,
                               const utils::Timeval& timeout);

    private:
        TLSShutdown(const std::string& instanceName,
                    SSL* ssl,
                    const std::function<void(void)>& onSuccess,
                    const std::function<void(void)>& onTimeout,
                    const std::function<void(int)>& onStatus,
                    const utils::Timeval& timeout);

        void readEvent() final;
        void writeEvent() final;
        void signalEvent(int signum) final;

        void readTimeout() final;
        void writeTimeout() final;

        void unobservedEvent() final;

        SSL* ssl = nullptr;
        std::function<void(void)> onSuccess;
        std::function<void(void)> onTimeout;
        std::function<void(int)> onStatus;

        bool timeoutTriggered;

        int fd = -1;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_TLSSHUTDOWN_H
