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

#ifndef NET_CONFIG_CONFIGPHYSICALSOCKET_H
#define NET_CONFIG_CONFIGPHYSICALSOCKET_H

#include "net/config/ConfigSection.h"
#include "net/phy/PhysicalSocketOption.h" // IWYU pragma: export

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <vector>

namespace CLI {
    class Option;
    class Validator;
} // namespace CLI

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    class ConfigPhysicalSocket {
    protected:
        explicit ConfigPhysicalSocket(net::config::ConfigSection* section);

    public:
        constexpr static std::string_view name{"socket"};
        constexpr static std::string_view description{"Configuration of socket behavior"};

        const std::map<int, std::map<int, net::phy::PhysicalSocketOption>>& getSocketOptions();

        ConfigPhysicalSocket& addSocketOption(int optLevel, int optName, int optValue);
        ConfigPhysicalSocket& addSocketOption(int optLevel, int optName, const std::string& optValue);
        ConfigPhysicalSocket& addSocketOption(int optLevel, int optName, const std::vector<char>& optValue);

        ConfigPhysicalSocket& removeSocketOption(int optLevel, int optName);

        ConfigPhysicalSocket& setRetry(bool retry = true);
        bool getRetry() const;

        ConfigPhysicalSocket& setRetryOnFatal(bool retry = true);
        bool getRetryOnFatal() const;

        ConfigPhysicalSocket& setRetryTimeout(double sec);
        double getRetryTimeout() const;

        ConfigPhysicalSocket& setRetryTries(unsigned int tries = 0); // 0 ... unlimmit
        unsigned int getRetryTries() const;

        ConfigPhysicalSocket& setRetryBase(double base);
        double getRetryBase() const;

        ConfigPhysicalSocket& setRetryLimit(unsigned int limit);
        unsigned int getRetryLimit() const;

        ConfigPhysicalSocket& setRetryJitter(double percent);
        double getRetryJitter() const;

    protected:
        CLI::Option* addSocketOption(const std::string& name,
                                     int optLevel,
                                     int optName,
                                     const std::string& description,
                                     const std::string& typeName,
                                     const std::string& defaultValue,
                                     const CLI::Validator& validator);

    private:
        net::config::ConfigSection* section;
        CLI::Option* retryOpt = nullptr;
        CLI::Option* retryOnFatalOpt = nullptr;
        CLI::Option* retryTriesOpt = nullptr;
        CLI::Option* retryTimeoutOpt = nullptr;
        CLI::Option* retryBaseOpt = nullptr;
        CLI::Option* retryLimitOpt = nullptr;
        CLI::Option* retryJitterOpt = nullptr;

        std::map<int, std::map<int, net::phy::PhysicalSocketOption>> socketOptionsMapMap;

        //        std::map<int, const net::phy::PhysicalSocketOption> socketOptionsMap; // key is optName, value is optLevel
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGPHYSICALSOCKET_H
