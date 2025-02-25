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

#ifndef NET_CONFIG_CONFIGTLS_H
#define NET_CONFIG_CONFIGTLS_H

#include "net/config/ConfigSection.h"

namespace net::config {
    class ConfigInstance;
}

#include "core/socket/stream/tls/ssl_utils.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class Option;
} // namespace CLI

#include "utils/Timeval.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    class ConfigTls : protected ConfigSection {
    protected:
        explicit ConfigTls(ConfigInstance* instance);

    public:
        ConfigTls& setInitTimeout(const utils::Timeval& newInitTimeout);
        utils::Timeval getInitTimeout() const;

        ConfigTls& setShutdownTimeout(const utils::Timeval& newShutdownTimeout);
        utils::Timeval getShutdownTimeout() const;

        ConfigTls& setCert(const std::string& cert);
        std::string getCert() const;

        ConfigTls& setCertKey(const std::string& certKey);
        std::string getCertKey() const;

        ConfigTls& setCertKeyPassword(const std::string& certKeyPassword);
        std::string getCertKeyPassword() const;

        ConfigTls& setCaCert(const std::string& caCert);
        std::string getCaCert() const;

        ConfigTls& setCaCertDir(const std::string& caCertDir);
        std::string getCaCertDir() const;

        ConfigTls& setCaCertUseDefaultDir(bool set = true);
        bool getCaCertUseDefaultDir() const;

        ConfigTls& setCaCertAcceptUnknown(bool set = true);
        bool getCaCertAcceptUnknown() const;

        ConfigTls& setCipherList(const std::string& cipherList);
        std::string getCipherList() const;

        ConfigTls& setSslOptions(ssl_option_t sslOptions);
        ssl_option_t getSslOptions() const;

        ConfigTls& setNoCloseNotifyIsEOF(bool noCloseNotifyIsEOF = true);
        bool getNoCloseNotifyIsEOF() const;

    private:
        CLI::Option* certOpt = nullptr;
        CLI::Option* certKeyOpt = nullptr;
        CLI::Option* certKeyPasswordOpt = nullptr;
        CLI::Option* caCertOpt = nullptr;
        CLI::Option* caCertDirOpt = nullptr;
        CLI::Option* caCertUseDefaultDirOpt = nullptr;
        CLI::Option* caCertAcceptUnknownOpt = nullptr;
        CLI::Option* cipherListOpt = nullptr;
        CLI::Option* sslOptionsOpt = nullptr;
        CLI::Option* initTimeoutOpt = nullptr;
        CLI::Option* shutdownTimeoutOpt = nullptr;
        CLI::Option* noCloseNotifyIsEOFOpt = nullptr;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGTLS_H
