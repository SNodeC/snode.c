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

#ifndef UTILS_CONFIGAPP_H
#define UTILS_CONFIGAPP_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
    class Formatter;
} // namespace CLI

#include <map>
#include <memory>
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
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    template <class T>
    class AppWithPtr : public CLI::App {
    public:
        AppWithPtr(const std::string& description, const std::string& name, T* t, bool manage);

        const T* getPtr() const;

        ~AppWithPtr() override;

        AppWithPtr* addStandardFlags(const std::function<void(CLI::App*)>& setShowConfigTriggerApp,
                                     const std::function<void(CLI::App*)>& setCommandlineTriggerApp);
        AppWithPtr* addSimpleHelp(const std::function<void(CLI::App*)>& setHelpTriggerApp);
        AppWithPtr* addHelp(const std::function<void(CLI::App*)>& setHelpTriggerApp);

        T* getPtr();

    private:
        T* ptr;
        bool manage;
    };

    template <class T>
    AppWithPtr<T>::AppWithPtr(const std::string& description, const std::string& name, T* t, bool manage)
        : CLI::App(description, name)
        , ptr(t)
        , manage(manage) {
    }

    template <class T>
    AppWithPtr<T>::~AppWithPtr() {
        if (manage) {
            delete ptr;
        }
    }

    template <class T>
    inline AppWithPtr<T>* AppWithPtr<T>::addStandardFlags(const std::function<void(CLI::App*)>& setShowConfigTriggerApp,
                                                          const std::function<void(CLI::App*)>& setCommandlineTriggerApp) {
        add_flag_function(
            "-s,--show-config",
            [setShowConfigTriggerApp, this](std::size_t) {
                setShowConfigTriggerApp(this);
            },
            "Show current configuration and exit")
            ->take_first()
            ->check(CLI::TypeValidator<bool>())
            ->disable_flag_override()
            ->configurable(false)
            ->trigger_on_parse()
            ->configurable(false);

        add_flag_function(
            "--command-line{standard}",
            [setCommandlineTriggerApp, this]([[maybe_unused]] std::int64_t count) {
                setCommandlineTriggerApp(this);
            },
            "Print command-line\n"
            "* standard (default): Show all non-default and required options\n"
            "* active: Show all active options\n"
            "* complete: Show the complete option set with default values\n"
            "* required: Show only required options")
            ->take_first()
            ->check(CLI::TypeValidator<bool>())
            ->disable_flag_override()
            ->trigger_on_parse();

        return this;
    }

    template <class T>
    inline AppWithPtr<T>* AppWithPtr<T>::addSimpleHelp(const std::function<void(CLI::App*)>& setHelpTriggerApp) {
        set_help_flag(
            "--help{exact},-h{exact}",
            [setHelpTriggerApp, this](std::size_t) {
                setHelpTriggerApp(this);
            },
            "Print help message and exit\n"
            "* standard: display help for the last command processed\n"
            "* exact: display help for the command directly preceding --help")
            ->take_first()
            ->check(CLI::IsMember({"standard", "exact"}))
            ->trigger_on_parse();

        return this;
    }

    template <class T>
    inline AppWithPtr<T>* AppWithPtr<T>::addHelp(const std::function<void(CLI::App*)>& setHelpTriggerApp) {
        set_help_flag(
            "-h{exact},--help{exact}",
            [setHelpTriggerApp, this](std::size_t) {
                setHelpTriggerApp(this);
            },
            "Print help message and exit\n"
            "* standard: display help for the last command processed\n"
            "* exact: display help for the command directly preceding --help\n"
            "* expanded: print help including all descendant command options")
            ->take_first()
            ->check(CLI::IsMember({"standard", "exact", "expanded"}))
            ->trigger_on_parse();

        return this;
    }

    template <class T>
    const T* AppWithPtr<T>::getPtr() const {
        return ptr;
    }

    template <class T>
    T* AppWithPtr<T>::getPtr() {
        return ptr;
    }

} // namespace utils

#endif // UTILS_CONFIGAPP_H
