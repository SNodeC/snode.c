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

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

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
        AppWithPtr(const std::string& description, const std::string& name, SubCommand* t);

        ~AppWithPtr() override;

        const SubCommand* getPtr() const;
        SubCommand* getPtr();

        void validate();

    private:
        SubCommand* ptr;
    };

    class SubCommand {
    protected:
        SubCommand(SubCommand* parent, std::shared_ptr<utils::AppWithPtr> appWithPtr, const std::string& group, bool final = false);

        template <typename ConcretSubCommand>
        SubCommand(SubCommand* parent, ConcretSubCommand* concretSubCommand, const std::string& group, bool final = false);

    public:
        SubCommand(const SubCommand&) = delete;
        SubCommand(SubCommand&&) = delete;

        SubCommand& operator=(const SubCommand&) = delete;
        SubCommand& operator=(SubCommand&&) = delete;

        virtual ~SubCommand();

        std::string getName() const;
        std::string version() const;

    protected:
        void parse(int argc, char* argv[]) const;

        SubCommand* description(const std::string& description);
        SubCommand* footer(const std::string& footer);

        void removeSubCommand();

    public:
        CLI::Option* setConfig(const std::string& defaultConfigFile) const;
        CLI::Option* setLogFile(const std::string& defaultLogFile) const;
        CLI::Option* setVersionFlag(const std::string& version) const;

        bool hasParent() const;
        SubCommand* getParent() const;

        SubCommand* allowExtras(bool allow = true);

        SubCommand* disabled(bool disabled = true);
        SubCommand* required(bool required = true);
        SubCommand* required(CLI::Option* option, bool required = true);

        bool getRequired() const {
            return subCommandApp->get_required();
        }

        SubCommand* needs(SubCommand* subCommand, bool needs = true);

        SubCommand* setRequireCallback(const std::function<void(void)>& callback);
        SubCommand* finalCallback(const std::function<void()>& finalCallback);

        std::string configToStr() const;
        std::string help(const CLI::App* helpApp, const CLI::AppFormatMode& mode) const;

        void addSubCommandApp(std::shared_ptr<utils::AppWithPtr> subCommand);

        template <typename NewSubCommand, typename... Args>
        NewSubCommand* newSubCommand(Args&&... args);

        template <typename RequestedSubCommand>
        RequestedSubCommand* getSubCommand();

        template <typename RequestedSubCommand>
        RequestedSubCommand* getSubCommand() const;

        CLI::Option* getOption(const std::string& name) const;

        CLI::Option* addOption(const std::string& name,
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

        CLI::Option* addOptionFunction(const std::string& name,
                                       const std::function<void(const std::string&)>& callback,
                                       const std::string& description,
                                       const std::string& typeName,
                                       const CLI::Validator& validator) const;

        template <typename ValueTypeT>
        CLI::Option* addOptionFunction(const std::string& name,
                                       const std::function<void(const std::string&)>& callback,
                                       const std::string& description,
                                       const std::string& typeName,
                                       ValueTypeT defaultValue,
                                       const CLI::Validator& validator) const;

        CLI::Option* addFlag(const std::string& name,
                             const std::string& description,
                             const std::string& typeName,
                             const CLI::Validator& validator) const;

        template <typename ValueTypeT>
        CLI::Option* addFlag(const std::string& name,
                             const std::string& description,
                             const std::string& typeName,
                             ValueTypeT defaultValue,
                             const CLI::Validator& validator) const;

        CLI::Option* addFlagFunction(const std::string& name,
                                     const std::function<void()>& callback,
                                     const std::string& description,
                                     const std::string& typeName,
                                     const CLI::Validator& validator) const;

        CLI::Option* addFlagFunction(const std::string& name,
                                     const std::function<void()>& callback,
                                     const std::string& description,
                                     const std::string& typeName,
                                     const std::string& defaultValue,
                                     const CLI::Validator& validator) const;

    protected:
        template <typename ValueTypeT>
        CLI::Option* setDefaultValue(CLI::Option* option, const ValueTypeT& value, bool clear = true) const;

        CLI::Option* setConfigurable(CLI::Option* option, bool configurable) const;

        static CLI::App* getHelpTriggerApp();
        static CLI::App* getShowConfigTriggerApp();
        static CLI::App* getCommandlineTriggerApp();

        static std::shared_ptr<CLI::Formatter> sectionFormatter;

        static std::map<std::string, std::string> aliases;

        static CLI::App* helpTriggerApp;
        static CLI::App* showConfigTriggerApp;
        static CLI::App* commandlineTriggerApp;

    private:
        CLI::Option* initialize(CLI::Option* option, const std::string& typeName, const CLI::Validator& validator, bool configurable) const;

        AppWithPtr* subCommandApp;
        std::string name;
        SubCommand* parent;

        std::shared_ptr<AppWithPtr> selfAnchoredSubCommandApp;
        std::set<SubCommand*> childSubCommands;

        bool final;

    protected:
        CLI::Option* helpOpt = nullptr;

    private:
        CLI::Option* showConfigOpt = nullptr;
        CLI::Option* commandlineOpt = nullptr;

        int requiredCount = 0;
        bool requiredDisabled = false;
    };

    template <typename ConcretSubCommand>
    inline SubCommand::SubCommand(SubCommand* parent, ConcretSubCommand* concretSubCommand, const std::string& group, bool final)
        : SubCommand(parent,
                     std::make_shared<utils::AppWithPtr>(
                         std::string(ConcretSubCommand::DESCRIPTION), std::string(ConcretSubCommand::NAME), concretSubCommand),
                     group,
                     final) {
    }

    template <typename NewSubCommand, typename... Args>
    NewSubCommand* SubCommand::newSubCommand(Args&&... args) {
        NewSubCommand* newSubCommand = nullptr;

        if (!final) {
            /*
            helpOpt->description("Print help message and exit\n"
                                 "* standard: display help for the last command processed\n"
                                 "* exact: display help for the command directly preceding --help\n"
                                 "* expanded: display help including all descendant sections");
            if (auto* existing = helpOpt->get_validator(); existing != nullptr) {
                *existing = std::move(CLI::IsMember({"standard", "exact", "expanded"})); // replace, do not append
            }
            */
            newSubCommand =
                dynamic_cast<NewSubCommand*>(*childSubCommands.insert(new NewSubCommand(this, std::forward<Args>(args)...)).first);
        }

        return newSubCommand;
    }

    template <typename RequestedSubCommand>
    RequestedSubCommand* SubCommand::getSubCommand() {
        auto* appWithPtr = subCommandApp->get_subcommand_no_throw(std::string(RequestedSubCommand::NAME));

        AppWithPtr* subCommandApp = dynamic_cast<utils::AppWithPtr*>(appWithPtr);

        return subCommandApp != nullptr ? dynamic_cast<RequestedSubCommand*>(subCommandApp->getPtr()) : nullptr;
    }

    template <typename RequestedSubCommand>
    RequestedSubCommand* SubCommand::getSubCommand() const {
        auto* appWithPtr = subCommandApp->get_subcommand_no_throw(std::string(RequestedSubCommand::NAME));

        AppWithPtr* subCommandApp = dynamic_cast<utils::AppWithPtr*>(appWithPtr);

        return subCommandApp != nullptr ? dynamic_cast<RequestedSubCommand*>(subCommandApp->getPtr()) : nullptr;
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addOption(const std::string& name,
                                       const std::string& description,
                                       const std::string& typeName,
                                       ValueTypeT defaultValue,
                                       const CLI::Validator& additionalValidator) const {
        return setDefaultValue(addOption(name, description, typeName, additionalValidator), defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addOptionVariable(const std::string& name,
                                               ValueTypeT& variable,
                                               const std::string& description,
                                               const std::string& typeName,
                                               const CLI::Validator& additionalValidator) const {
        return initialize(
            subCommandApp->add_option(name, variable, description), typeName, additionalValidator, !subCommandApp->get_disabled());
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addOptionVariable(const std::string& name,
                                               ValueTypeT& variable,
                                               const std::string& description,
                                               const std::string& typeName,
                                               ValueTypeT defaultValue,
                                               const CLI::Validator& additionalValidator) const {
        return setDefaultValue(addOptionVariable(name, variable, description, typeName, additionalValidator), defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addOptionFunction(const std::string& name,
                                               const std::function<void(const std::string&)>& callback,
                                               const std::string& description,
                                               const std::string& typeName,
                                               ValueTypeT defaultValue,
                                               const CLI::Validator& validator) const {
        return setDefaultValue(addOptionFunction(name, callback, description, typeName, validator), defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::addFlag(const std::string& name,
                                     const std::string& description,
                                     const std::string& typeName,
                                     ValueTypeT defaultValue,
                                     const CLI::Validator& additionalValidator) const {
        return setDefaultValue(addFlag(name, description, typeName, additionalValidator), defaultValue);
    }

    template <typename ValueTypeT>
    CLI::Option* SubCommand::setDefaultValue(CLI::Option* option, const ValueTypeT& value, bool clear) const {
        try {
            option->default_val(value);

            if (clear) {
                option->clear();
            }
        } catch (const CLI::ParseError&) {
            option = nullptr;
        }

        return option;
    }

} // namespace utils

#endif // UTILS_SUBCOMMAND_H
