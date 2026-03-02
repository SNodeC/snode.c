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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

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
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {
    class SubCommand;

    class AppWithPtr : public CLI::App {
    public:
        AppWithPtr(const std::string& description, const std::string& name, SubCommand* t, bool manage);

        const SubCommand* getPtr() const;

        ~AppWithPtr() override;

        SubCommand* getPtr();

    private:
        SubCommand* ptr;
        bool manage;
    };

} // namespace utils

namespace net::config {

    template <typename T>
    std::shared_ptr<utils::AppWithPtr> Instance(const std::string& name, const std::string& description, T* section, bool manage = false);

}

namespace utils {

    class SubCommand {
    protected:
        SubCommand(std::shared_ptr<utils::AppWithPtr> appWithPtr, bool final = true);

    public:
        SubCommand(const SubCommand&) = delete;
        SubCommand(SubCommand&&) = delete;

        virtual ~SubCommand();

        SubCommand& operator=(const SubCommand&) = delete;
        SubCommand& operator=(SubCommand&&) = delete;

        std::string getName();
        std::string version();

        void parse(int argc, char* argv[]);

        SubCommand* description(const std::string& description);
        SubCommand* footer(const std::string& footer);

        CLI::Option* setConfig(const std::string& defaultConfigFile) const;
        CLI::Option* setLogFile(const std::string& defaultLogFile) const;
        CLI::Option* setVersionFlag(const std::string& version) const;

        bool hasParent() const;
        SubCommand* getParent();
        SubCommand* setRequireCallback(const std::function<void(void)>& callback);

        SubCommand* allowExtras(bool allow = true);
        SubCommand* required(bool required = true, bool force = true);
        SubCommand* required(SubCommand* subCommand, bool required = true);
        SubCommand* required(CLI::Option* option, bool required = true);

        SubCommand* needs(SubCommand* subCommand, bool needs = true);

        SubCommand* disabled(SubCommand* subCommand, bool disabled = true);
        SubCommand* finalCallback(const std::function<void()>& finalCallback);

        std::string configToStr() const;
        std::string help(const CLI::App* helpApp, const CLI::AppFormatMode& mode) const;

        std::shared_ptr<utils::AppWithPtr> newSubCommand(std::shared_ptr<utils::AppWithPtr> appWithPtr, const std::string& group) const;

        template <typename T>
        T* addInstance();

        template <typename T>
        T* getInstance();

        SubCommand* removeSubCommand(utils::SubCommand* subCommand);

    protected:
        CLI::Option* setConfigurable(CLI::Option* option, bool configurable) const;

    private:
        CLI::Option* initialize(CLI::Option* option, const std::string& typeName, const CLI::Validator& validator, bool configurable) const;

    public:
        CLI::Option* getOption(const std::string& name) const;

        CLI::Option* addOption(const std::string& name,
                               const std::string& description,
                               const std::string& typeName,
                               const CLI::Validator& validator) const;

        CLI::Option* addOptionFunction(const std::string& name,
                                       std::function<void(const std::string&)>& callback,
                                       const std::string& description,
                                       const std::string& typeName,
                                       const CLI::Validator& validator) const;

        CLI::Option* addFlag(const std::string& name,
                             const std::string& description,
                             const std::string& typeName,
                             const CLI::Validator& validator) const;

        CLI::Option* addFlagFunction(const std::string& name,
                                     const std::function<void(std::int64_t)>& callback,
                                     const std::string& description,
                                     const std::string& typeName,
                                     const CLI::Validator& validator) const;

        template <typename ValueTypeT>
        CLI::Option* addOption(const std::string& name,
                               const std::string& description,
                               const std::string& typeName,
                               ValueTypeT defaultValue,
                               const CLI::Validator& validator) const;

        template <typename ValueTypeT>
        CLI::Option* addOptionVariable(const std::string& name,
                                       ValueTypeT& variable,
                                       const std::string& description,
                                       const std::string& typeName,
                                       const CLI::Validator& additionalValidator) const;

        template <typename ValueTypeT>
        CLI::Option* addOptionVariable(const std::string& name,
                                       ValueTypeT& variable,
                                       const std::string& description,
                                       const std::string& typeName,
                                       ValueTypeT defaultValue,
                                       const CLI::Validator& additionalValidator) const;

        template <typename ValueTypeT>
        CLI::Option* addOptionFunction(const std::string& name,
                                       std::function<void(const std::string&)>& callback,
                                       const std::string& description,
                                       const std::string& typeName,
                                       ValueTypeT defaultValue,
                                       const CLI::Validator& validator) const;

        template <typename ValueTypeT>
        CLI::Option* addFlag(const std::string& name,
                             const std::string& description,
                             const std::string& typeName,
                             ValueTypeT defaultValue,
                             const CLI::Validator& validator) const;

        template <typename ValueTypeT>
        CLI::Option* addFlagFunction(const std::string& name,
                                     const std::function<void(std::int64_t)>& callback,
                                     const std::string& description,
                                     const std::string& typeName,
                                     ValueTypeT defaultValue,
                                     const CLI::Validator& validator) const;

        template <typename ValueTypeT>
        static CLI::Option* setDefaultValue(CLI::Option* option, const ValueTypeT& value, bool clear = true);

        static CLI::App* getHelpTriggerApp();
        static CLI::App* getShowConfigTriggerApp();
        static CLI::App* getCommandlineTriggerApp();

        static std::shared_ptr<CLI::Formatter> sectionFormatter;

    private:
        std::shared_ptr<utils::AppWithPtr> subCommandSc;

    protected:
        bool final;
        std::map<std::string, std::string> aliases;

        static CLI::App* helpTriggerApp;
        static CLI::App* showConfigTriggerApp;
        static CLI::App* commandlineTriggerApp;

        CLI::Option* helpOpt = nullptr;
        CLI::Option* showConfigOpt = nullptr;
        CLI::Option* commandlineOpt = nullptr;

        std::vector<std::shared_ptr<utils::AppWithPtr>> configInstances; // Store anything

        int requiredCount = 0;
    };

    template <typename ConcreteSubCommand>
    ConcreteSubCommand* SubCommand::addInstance() {
        return !final
                   ? dynamic_cast<ConcreteSubCommand*>(configInstances
                                                           .emplace_back(net::config::Instance(std::string(ConcreteSubCommand::NAME),
                                                                                               std::string(ConcreteSubCommand::DESCRIPTION),
                                                                                               new ConcreteSubCommand(this),
                                                                                               true))
                                                           ->getPtr())
                   : nullptr;
    }

    template <typename ConcreteSubCommand>
    ConcreteSubCommand* SubCommand::getInstance() {
        auto* appWithPtr = subCommandSc->get_subcommand_no_throw(std::string(ConcreteSubCommand::NAME));

        AppWithPtr* subCommandApp = dynamic_cast<utils::AppWithPtr*>(appWithPtr);

        return subCommandApp != nullptr ? dynamic_cast<ConcreteSubCommand*>(subCommandApp->getPtr()) : nullptr;
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addOptionFunction(const std::string& name,
                                               std::function<void(const std::string&)>& callback,
                                               const std::string& description,
                                               const std::string& typeName,
                                               ValueTypeT defaultValue,
                                               const CLI::Validator& validator) const {
        return addOptionFunction(name, callback, description, typeName, validator) //
            ->default_val(defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addOption(const std::string& name,
                                       const std::string& description,
                                       const std::string& typeName,
                                       ValueTypeT defaultValue,
                                       const CLI::Validator& additionalValidator) const {
        return addOption(name, description, typeName, additionalValidator) //
            ->default_val(defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addOptionVariable(const std::string& name,
                                               ValueTypeT& variable,
                                               const std::string& description,
                                               const std::string& typeName,
                                               const CLI::Validator& additionalValidator) const {
        return initialize(
            subCommandSc->add_option(name, variable, description), typeName, additionalValidator, !subCommandSc->get_disabled());
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addOptionVariable(const std::string& name,
                                               ValueTypeT& variable,
                                               const std::string& description,
                                               const std::string& typeName,
                                               ValueTypeT defaultValue,
                                               const CLI::Validator& additionalValidator) const {
        return addOption(name, variable, description, typeName, additionalValidator) //
            ->default_val(defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addFlag(const std::string& name,
                                     const std::string& description,
                                     const std::string& typeName,
                                     ValueTypeT defaultValue,
                                     const CLI::Validator& additionalValidator) const {
        return addFlag(name, description, typeName, additionalValidator) //
            ->default_val(defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addFlagFunction(const std::string& name,
                                             const std::function<void(std::int64_t)>& callback,
                                             const std::string& description,
                                             const std::string& typeName,
                                             ValueTypeT defaultValue,
                                             const CLI::Validator& validator) const {
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

namespace net::config {

    template <typename T>
    std::shared_ptr<utils::AppWithPtr> Instance(const std::string& name, const std::string& description, T* section, bool manage) {
        std::shared_ptr<utils::AppWithPtr> subCommandSc = std::make_shared<utils::AppWithPtr>(description, name, section, manage);

        subCommandSc->option_defaults()->take_last();
        subCommandSc->formatter(utils::SubCommand::sectionFormatter);

        return subCommandSc;
    }
} // namespace net::config

#endif // UTILS_SUBCOMMAND_H
