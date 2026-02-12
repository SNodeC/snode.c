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

#ifndef NET_CONFIG_CONFIGSECTION_H
#define NET_CONFIG_CONFIGSECTION_H

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Config.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string> // IWYU pragma: export

namespace CLI {
    class App;
    class Option;
    class Validator;
} // namespace CLI

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    class ConfigSection {
    public:
        //        ConfigSection(ConfigInstance* instance, const std::string& name, const std::string& description);

        ConfigSection(ConfigInstance* instanceSc, std::shared_ptr<CLI::App> sectionApp);

        virtual ~ConfigSection() = default;

        ConfigSection(const ConfigSection&) = delete;
        ConfigSection(ConfigSection&&) = delete;

        ConfigSection& operator=(const ConfigSection&) = delete;
        ConfigSection& operator=(ConfigSection&&) = delete;

        CLI::Option* addOption(const std::string& name, const std::string& description);

        CLI::Option* addOption(const std::string& name, const std::string& description, const std::string& typeName);

        CLI::Option* addOption(const std::string& name,
                               const std::string& description,
                               const std::string& typeName,
                               const CLI::Validator& additionalValidator);

        template <typename ValueTypeT>
        CLI::Option*
        addOption(const std::string& name, const std::string& description, const std::string& typeName, ValueTypeT defaultValue);

        template <typename ValueTypeT>
        CLI::Option* addOption(const std::string& name,
                               const std::string& description,
                               const std::string& typeName,
                               ValueTypeT defaultValue,
                               const CLI::Validator& additionalValidator);

        CLI::Option* addFlag(const std::string& name, const std::string& description, const std::string& typeName);

        CLI::Option* addFlag(const std::string& name,
                             const std::string& description,
                             const std::string& typeName,
                             const CLI::Validator& additionalValidator);

        template <typename ValueTypeT>
        CLI::Option* addFlag(const std::string& name, const std::string& description, const std::string& typeName, ValueTypeT defaultValue);

        template <typename ValueTypeT>
        CLI::Option* addFlag(const std::string& name,
                             const std::string& description,
                             const std::string& typeName,
                             ValueTypeT defaultValue,
                             const CLI::Validator& additionalValidator);

        CLI::Option* addFlagFunction(const std::string& name,
                                     const std::function<void()>& callback,
                                     const std::string& description,
                                     const std::string& typeName,
                                     const std::string& defaultValue);

        CLI::Option* addFlagFunction(const std::string& name,
                                     const std::function<void()>& callback,
                                     const std::string& description,
                                     const std::string& typeName,
                                     const std::string& defaultValue,
                                     const CLI::Validator& validator);

        CLI::Option* getOption(const std::string& name) const;

        void required(CLI::Option* opt, bool req = true);

        bool required() const;

    protected:
        void setConfigurable(CLI::Option* option, bool configurable);

        CLI::App* sectionSc = nullptr;

    private:
        ConfigInstance* instanceSc = nullptr;

        uint8_t requiredCount = 0;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGSECTION_H
