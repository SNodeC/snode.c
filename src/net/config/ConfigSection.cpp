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

#include "net/config/ConfigSection.h"

#include "net/config/ConfigInstance.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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

#include "utils/Config.h"

#include <memory>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    ConfigSection::ConfigSection(ConfigInstance* instanceSc, std::shared_ptr<CLI::App> sectionApp, const std::string& group)
        : instanceSc(instanceSc) {
        sectionSc = instanceSc->addSection(sectionApp, group);
        sectionSc->description(sectionSc->get_description() + " for instance '" + instanceSc->getInstanceName() + "'");
    }

    void ConfigSection::required(CLI::Option* opt, bool req) {
        if (req != opt->get_required()) {
            if (req) {
                ++requiredCount;
                sectionSc->needs(opt);
                opt->default_str("");
            } else {
                --requiredCount;
                sectionSc->remove_needs(opt);
            }

            opt->required(req);

            instanceSc->required(sectionSc, requiredCount > 0);
        }
    }

    bool ConfigSection::required() const {
        return requiredCount > 0;
    }

    void ConfigSection::setConfigurable(CLI::Option* option, bool configurable) {
        option->configurable(configurable);
        option->group(sectionSc->get_formatter()->get_label(configurable ? "Persistent Options" : "Nonpersistent Options"));
    }

    CLI::Option* ConfigSection::addOption(const std::string& name, const std::string& description) {
        if (!instanceSc->getInstanceName().empty() && !sectionSc->get_display_name().empty()) {
            sectionSc->disabled(false);
        }

        CLI::Option* opt = sectionSc //
                               ->add_option(name, description);

        if (opt->get_configurable()) {
            opt->group(sectionSc->get_formatter()->get_label("Persistent Options"));
        }

        return opt;
    }

    CLI::Option* ConfigSection::addOption(const std::string& name, const std::string& description, const std::string& typeName) {
        return addOption(name, description) //
            ->type_name(typeName);
    }

    CLI::Option* ConfigSection::addOption(const std::string& name,
                                          const std::string& description,
                                          const std::string& typeName,
                                          const CLI::Validator& additionalValidator) {
        return addOption(name, description, typeName) //
            ->check(additionalValidator);
    }

    CLI::Option* ConfigSection::addFlag(const std::string& name, const std::string& description) {
        sectionSc->disabled(false);

        CLI::Option* opt = sectionSc //
                               ->add_flag(name, description);

        if (opt->get_configurable()) {
            opt->group(sectionSc->get_formatter()->get_label("Persistent Options"));
        }

        return opt;
    }

    CLI::Option* ConfigSection::addFlag(const std::string& name, const std::string& description, const std::string& typeName) {
        return addFlag(name, description)->type_name(typeName);
    }

    CLI::Option* ConfigSection::addFlag(const std::string& name,
                                        const std::string& description,
                                        const std::string& typeName,
                                        const CLI::Validator& additionalValidator) {
        return addFlag(name, description, typeName) //
            ->check(additionalValidator);
    }

    CLI::Option* ConfigSection::addFlagFunction(const std::string& name,
                                                const std::function<void()>& callback,
                                                const std::string& description,
                                                const std::string& typeName,
                                                const std::string& defaultValue) {
        sectionSc->disabled(false);

        CLI::Option* opt = sectionSc //
                               ->add_flag_function(
                                   name,
                                   [callback](std::int64_t) {
                                       callback();
                                   },
                                   description)
                               ->default_val(defaultValue)
                               ->type_name(typeName)
                               ->take_last();
        if (opt->get_configurable()) {
            opt->group(sectionSc->get_formatter()->get_label("Persistent Options"));
        }

        return opt;
    }

    CLI::Option* ConfigSection::addFlagFunction(const std::string& name,
                                                const std::function<void()>& callback,
                                                const std::string& description,
                                                const std::string& typeName,
                                                const std::string& defaultValue,
                                                const CLI::Validator& validator) {
        return addFlagFunction(name, callback, description, typeName, defaultValue) //
            ->check(validator);
    }

    CLI::Option* ConfigSection::getOption(const std::string& name) const {
        return sectionSc->get_option(name);
    }

} // namespace net::config
