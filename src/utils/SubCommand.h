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

#ifndef UTILS_SUBCOMMAND_H
#define UTILS_SUBCOMMAND_H

#include <map>    // for map
#include <memory> // for shared_ptr

namespace utils {
    template <class T>
    struct AppWithPtr;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <ostream>
#include <string>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#ifdef __has_warning
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#if __has_warning("-Wmissing-noreturn")
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif
#if __has_warning("-Wnrvo")
#pragma GCC diagnostic ignored "-Wnrvo"
#endif
#endif
#endif
#include "utils/CLI11.hpp"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "log/Logger.h"

#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    class SubCommand {
    public:
        SubCommand(SubCommand* subCommand);

        SubCommand* required(bool required = true, bool force = true);

        SubCommand* setRequireCallback(const std::function<void(void)>& callback);

        CLI::App* getParent() const;

        SubCommand* addStandardFlags();
        SubCommand* addSimpleHelp();
        SubCommand* addHelp();

        CLI::App* getHelpTriggerApp();
        CLI::App* getShowConfigTriggerApp();
        CLI::App* getCommandlineTriggerApp();

        SubCommand* required(SubCommand* instance, bool required = true);
        SubCommand* required(CLI::Option* option, bool required = true);

        SubCommand* disabled(SubCommand* instance, bool disabled = true);

        utils::SubCommand*
        newInstance(std::shared_ptr<utils::AppWithPtr<SubCommand>> appWithPtr, const std::string& group, bool final = false);
        SubCommand* removeInstance(utils::SubCommand* instance);

    protected:
        CLI::Option* setConfigurable(CLI::Option* option, bool configurable) const;

    private:
        CLI::Option* initialize(CLI::Option* option, const std::string& typeName, const CLI::Validator& validator, bool configurable) const;

    public:
        template <typename T>
        T* addInstance();

        template <typename T>
        T* getInstance();

        CLI::Option* getOption(const std::string& name) const;

        CLI::Option*
        addOption(const std::string& name, const std::string& description, const std::string& typeName, const CLI::Validator& validator);

        CLI::Option* addOptionFunction(const std::string& name,
                                       std::function<void(const std::string&)>& callback,
                                       const std::string& description,
                                       const std::string& typeName,
                                       const CLI::Validator& validator);

        CLI::Option*
        addFlag(const std::string& name, const std::string& description, const std::string& typeName, const CLI::Validator& validator);

        CLI::Option* addFlagFunction(const std::string& name,
                                     const std::function<void(std::int64_t)>& callback,
                                     const std::string& description,
                                     const std::string& typeName,
                                     const CLI::Validator& validator);

        template <typename ValueTypeT>
        CLI::Option* addOption(const std::string& name,
                               const std::string& description,
                               const std::string& typeName,
                               ValueTypeT defaultValue,
                               const CLI::Validator& validator);

        template <typename ValueTypeT>
        CLI::Option* addOptionVariable(const std::string& name,
                                       ValueTypeT& variable,
                                       const std::string& description,
                                       const std::string& typeName,
                                       const CLI::Validator& additionalValidator);

        template <typename ValueTypeT>
        CLI::Option* addOptionVariable(const std::string& name,
                                       ValueTypeT& variable,
                                       const std::string& description,
                                       const std::string& typeName,
                                       ValueTypeT defaultValue,
                                       const CLI::Validator& additionalValidator);

        template <typename ValueTypeT>
        CLI::Option* addOptionFunction(const std::string& name,
                                       std::function<void(const std::string&)>& callback,
                                       const std::string& description,
                                       const std::string& typeName,
                                       ValueTypeT defaultValue,
                                       const CLI::Validator& validator);

        template <typename ValueTypeT>
        CLI::Option* addFlag(const std::string& name,
                             const std::string& description,
                             const std::string& typeName,
                             ValueTypeT defaultValue,
                             const CLI::Validator& validator);

        template <typename ValueTypeT>
        CLI::Option* addFlagFunction(const std::string& name,
                                     const std::function<void(std::int64_t)>& callback,
                                     const std::string& description,
                                     const std::string& typeName,
                                     ValueTypeT defaultValue,
                                     const CLI::Validator& validator);

        template <typename ValueTypeT>
        static CLI::Option* setDefaultValue(CLI::Option* option, const ValueTypeT& value, bool clear = true);

    protected:
        CLI::App* subCommandSc = nullptr;
        std::map<std::string, std::string> aliases;

    public:
        static std::shared_ptr<CLI::Formatter> sectionFormatter;

    private:
        CLI::App* helpTriggerApp = nullptr;
        CLI::App* showConfigTriggerApp = nullptr;
        CLI::App* commandlineTriggerApp = nullptr;

        std::vector<std::shared_ptr<SubCommand>> configInstances; // Store anything

        int requiredCount = 0;
    };

    template <typename T>
    T* SubCommand::addInstance() {
        return std::static_pointer_cast<T>(configInstances.emplace_back(std::make_shared<T>(this))).get();
    }

    template <typename T>
    T* SubCommand::getInstance() {
        auto* appWithPtr = subCommandSc->get_subcommand_no_throw(std::string(T::name));

        AppWithPtr<T>* instanceApp = dynamic_cast<utils::AppWithPtr<T>*>(appWithPtr);

        return instanceApp != nullptr ? instanceApp->getPtr() : nullptr;
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addOptionFunction(const std::string& name,
                                               std::function<void(const std::string&)>& callback,
                                               const std::string& description,
                                               const std::string& typeName,
                                               ValueTypeT defaultValue,
                                               const CLI::Validator& validator) {
        return addOptionFunction(name, callback, description, typeName, validator) //
            ->default_val(defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addOption(const std::string& name,
                                       const std::string& description,
                                       const std::string& typeName,
                                       ValueTypeT defaultValue,
                                       const CLI::Validator& additionalValidator) {
        return addOption(name, description, typeName, additionalValidator) //
            ->default_val(defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addOptionVariable(const std::string& name,
                                               ValueTypeT& variable,
                                               const std::string& description,
                                               const std::string& typeName,
                                               const CLI::Validator& additionalValidator) {
        return initialize(
            subCommandSc->add_option(name, variable, description), typeName, additionalValidator, !subCommandSc->get_disabled());
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addOptionVariable(const std::string& name,
                                               ValueTypeT& variable,
                                               const std::string& description,
                                               const std::string& typeName,
                                               ValueTypeT defaultValue,
                                               const CLI::Validator& additionalValidator) {
        return addOption(name, variable, description, typeName, additionalValidator) //
            ->default_val(defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addFlag(const std::string& name,
                                     const std::string& description,
                                     const std::string& typeName,
                                     ValueTypeT defaultValue,
                                     const CLI::Validator& additionalValidator) {
        return addFlag(name, description, typeName, additionalValidator) //
            ->default_val(defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addFlagFunction(const std::string& name,
                                             const std::function<void(std::int64_t)>& callback,
                                             const std::string& description,
                                             const std::string& typeName,
                                             ValueTypeT defaultValue,
                                             const CLI::Validator& validator) {
        return addFlagFunction(name, callback, description, typeName, validator) //
            ->default_val(defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::setDefaultValue(CLI::Option* option, const ValueTypeT& value, bool clear) {
        try {
            option->default_val(value);

            if (clear) {
                option->clear();
            }
        } catch (const CLI::ParseError& e) {
            LOG(ERROR) << std::string{"["} << Color::Code::FG_RED << e.get_name() << Color::Code::FG_DEFAULT << "] " << e.what()
                       << std::endl;
        }

        return option;
    }

} // namespace utils

#endif // UTILS_SUBCOMMAND_H
