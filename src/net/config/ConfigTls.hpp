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

#include "net/config/ConfigSection.hpp"
#include "net/config/ConfigTls.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    template <typename ConcretConfigTls>
    ConfigTls::ConfigTls(ConfigInstance* instance, ConcretConfigTls section)
        : ConfigSection(instance, section) {
        certOpt = addOption( //
            "--cert",
            "Certificate chain file",
            "filename",
            CLI::ExistingFile.description("PEM-FILE"));

        certKeyOpt = addOption( //
            "--cert-key",
            "Certificate key file",
            "filename",
            CLI::ExistingFile.description("PEM-FILE"));

        certKeyPasswordOpt = addOption( //
            "--cert-key-password",
            "Password for the certificate key file",
            "password",
            "",
            CLI::TypeValidator<std::string>());

        caCertOpt = addOption( //
            "--ca-cert",
            "CA-certificate file",
            "filename",
            CLI::ExistingFile.description("PEM-FILE"));

        caCertDirOpt = addOption( //
            "--ca-cert-dir",
            "CA-certificate directory",
            "directory",
            CLI::ExistingDirectory.description("PEM-CONTAINER-DIR"));

        caCertUseDefaultDirOpt = addFlag( //
            "--ca-cert-use-default-dir{true}",
            "Use default CA-certificate directory",
            "bool",
            "false",
            CLI::IsMember({"true", "false"}));

        caCertAcceptUnknownOpt = addFlag( //
            "--ca-cert-accept-unknown{true}",
            "Accept unknown certificates (unsecure)",
            "bool",
            "false",
            CLI::IsMember({"true", "false"}));

        cipherListOpt = addOption( //
            "--cipher-list",
            "Cipher list (OpenSSL syntax)",
            "cipher_list",
            "",
            CLI::TypeValidator<std::string>("CIPHER"));

        sslOptionsOpt = addOption( //
            "--ssl-options",
            "OR combined SSL/TLS options (OpenSSL values)",
            "options",
            0,
            CLI::TypeValidator<ssl_option_t>());

        initTimeoutOpt = addOption( //
            "--init-timeout",
            "SSL/TLS initialization timeout in seconds",
            "timeout",
            tlsInitTimeout,
            CLI::PositiveNumber);

        shutdownTimeoutOpt = addOption( //
            "--shutdown-timeout",
            "SSL/TLS shutdown timeout in seconds",
            "timeout",
            tlsShutdownTimeout,
            CLI::PositiveNumber);
    }

} // namespace net::config
