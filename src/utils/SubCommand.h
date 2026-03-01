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

#include "ConfigApp.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    class SubCommand {
    protected:
        SubCommand(std::shared_ptr<utils::AppWithPtr<SubCommand>> appWithPtr);

    public:
        SubCommand(const SubCommand&) = delete;
        SubCommand(SubCommand&&) = delete;

        SubCommand& operator=(const SubCommand&) = delete;
        SubCommand& operator=(SubCommand&&) = delete;

        SubCommand* required(bool required = true, bool force = true);

        SubCommand* setRequireCallback(const std::function<void(void)>& callback);

        CLI::App* getParent() const;

        SubCommand* addStandardFlags();
        SubCommand* addSimpleHelp();
        SubCommand* addHelp();

        SubCommand* getHelpTriggerApp();
        SubCommand* getShowConfigTriggerApp();
        SubCommand* getCommandlineTriggerApp();

        SubCommand* required(SubCommand* instance, bool required = true);
        SubCommand* required(CLI::Option* option, bool required = true);

        SubCommand* disabled(SubCommand* instance, bool disabled = true);

        std::shared_ptr<utils::AppWithPtr<SubCommand>>
        newInstance(std::shared_ptr<utils::AppWithPtr<SubCommand>> appWithPtr, const std::string& group, bool final = false);

        template <typename T>
        T* addInstance(const std::string& group = "Application", bool final = false);

        template <typename T>
        T* getInstance();

        SubCommand* removeInstance(utils::SubCommand* instance);

    protected:
        CLI::Option* setConfigurable(CLI::Option* option, bool configurable) const;

    private:
        CLI::Option* initialize(CLI::Option* option, const std::string& typeName, const CLI::Validator& validator, bool configurable) const;

    public:
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
        SubCommand* parent;

        std::shared_ptr<utils::AppWithPtr<SubCommand>> subCommandSc;
        std::map<std::string, std::string> aliases;

    public:
        static std::shared_ptr<CLI::Formatter> sectionFormatter;

    private:
        static SubCommand* helpTriggerSubCommand;
        static SubCommand* showConfigTriggerSubCommand;
        static SubCommand* commandlineTriggerSubCommand;

        std::vector<std::shared_ptr<SubCommand>> configInstances; // Store anything

        int requiredCount = 0;
    };

    template <typename ConcreteSubCommand>
    ConcreteSubCommand* SubCommand::addInstance([[maybe_unused]] const std::string& group, [[maybe_unused]] bool final) {
        /*
        return configInstances
            .emplace_back(newInstance(net::config::Instance(std::string(ConcreteSubCommand::name),
                                                            std::string(ConcreteSubCommand::description),
                                                            new ConcreteSubCommand()),
                                      group,
                                      final))
            ->getPtr();
        */
        return nullptr;
    }

    template <typename ConcreteSubCommand>
    ConcreteSubCommand* SubCommand::getInstance() {
        return nullptr;
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
