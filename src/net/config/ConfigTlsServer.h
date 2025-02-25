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

#ifndef NET_CONFIG_CONFIGTLSSERVER_H
#define NET_CONFIG_CONFIGTLSSERVER_H

#include "net/config/ConfigTls.h" // IWYU pragma: export

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>
#include <variant>

namespace CLI {
    class Option;
}

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    class ConfigTlsServer : public ConfigTls {
    private:
        using Super = ConfigTls;

    public:
        using Tls = ConfigTlsServer;

    protected:
        explicit ConfigTlsServer(ConfigInstance* instance);

    public:
        ConfigTlsServer& setForceSni(bool forceSni = true);
        bool getForceSni() const;

        ConfigTlsServer&
        addSniCerts(const std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>>& sniCerts);
        ConfigTlsServer& addSniCert(const std::string& domain,
                                    const std::map<std::string, std::variant<std::string, bool, ssl_option_t>>& sniCert);
        const std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>>& getSniCerts();

    private:
        std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>> defaultSniCerts;
        std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>> configuredSniCerts;

        CLI::Option* sniCertsOpt = nullptr;
        CLI::Option* forceSniOpt = nullptr;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGTLSSERVER_H
