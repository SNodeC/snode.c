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

#ifndef NET_CONFIG_CONFIGINSTANCE_H
#define NET_CONFIG_CONFIGINSTANCE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

namespace net::config {
    class ConfigSection;
}

#include "utils/Config.h"

#include <cstdint>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    class ConfigInstance {
    public:
        //        using Instance = ConfigInstance;

        enum class Role { SERVER, CLIENT };

    protected:
        explicit ConfigInstance(const std::string& instanceName, Role role);

        virtual ~ConfigInstance();

    public:
        ConfigInstance(ConfigInstance&) = delete;
        ConfigInstance(ConfigInstance&&) = delete;

        ConfigInstance& operator=(ConfigInstance&) = delete;
        ConfigInstance& operator=(ConfigInstance&&) = delete;

        Role getRole();

        const std::string& getInstanceName() const;
        void setInstanceName(const std::string& instanceName);

        bool getDisabled() const;
        void setDisabled(bool disabled = true);

        CLI::App* addSection(const std::string& name, const std::string& description, const std::string& group = "Sections");

        CLI::App* addSection(std::shared_ptr<CLI::App> appWithPtr, const std::string& group = "Sections");

    private:
        CLI::App* getSection(const std::string& name, bool onlyGot = false, bool recursive = false) const;

    public:
        template <typename SectionTypeT>
        SectionTypeT* getSection(const std::string& name, bool onlyGot = false, bool recursive = false) const;

        bool gotSection(const std::string& name, bool recursive = false) const;

        void required(CLI::App* section, bool req = true);
        bool getRequired() const;

        CLI::App* get() const;

        void configurable(bool configurable = true);

    private:
        uint8_t requiredCount = 0;

        std::string instanceName;
        static const std::string nameAnonymous;

        Role role;

        CLI::App* instanceSc = nullptr;
        CLI::Option* disableOpt = nullptr;

        friend class net::config::ConfigSection;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGINSTANCE_H
