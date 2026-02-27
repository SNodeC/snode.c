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

#include <cstdint>
#include <memory>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    ConfigSection::~ConfigSection() {
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

            required(requiredCount > 0);
        }
    }

    void ConfigSection::required(bool required) {
        instance->required(sectionSc, required);
    }

    bool ConfigSection::getRequired() const {
        return requiredCount > 0;
    }

    CLI::Option* ConfigSection::getOption(const std::string& name) const {
        return sectionSc->get_option(name);
    }

    CLI::Option* ConfigSection::addOption(const std::string& name,
                                          const std::string& description,
                                          const std::string& typeName,
                                          const CLI::Validator& validator) {
        return initialize(sectionSc //
                              ->add_option(name, description),
                          typeName,
                          validator,
                          !sectionSc->get_disabled());
    }

    CLI::Option* ConfigSection::addOptionFunction(const std::string& name,
                                                  std::function<void(const std::string&)>& callback,
                                                  const std::string& description,
                                                  const std::string& typeName,
                                                  const CLI::Validator& validator) {
        return initialize(sectionSc //
                              ->add_option_function(name, callback, description),
                          typeName,
                          validator,
                          !sectionSc->get_disabled());
    }

    CLI::Option* ConfigSection::addFlag(const std::string& name,
                                        const std::string& description,
                                        const std::string& typeName,
                                        const CLI::Validator& validator) {
        return initialize(sectionSc //
                              ->add_flag(name, description),
                          typeName,
                          validator,
                          !sectionSc->get_disabled());
    }

    CLI::Option* ConfigSection::addFlagFunction(const std::string& name,
                                                const std::function<void(std::int64_t)>& callback,
                                                const std::string& description,
                                                const std::string& typeName,
                                                const CLI::Validator& validator) {
        return initialize(sectionSc //
                              ->add_flag_function(name, callback, description),
                          typeName,
                          validator,
                          !sectionSc->get_disabled());
    }

    CLI::Option* ConfigSection::setConfigurable(CLI::Option* option, bool configurable) const {
        option //
            ->configurable(configurable)
            ->group(sectionSc->get_formatter()->get_label(configurable ? "Persistent Options" : "Nonpersistent Options"));

        return option;
    }

    CLI::Option*
    ConfigSection::initialize(CLI::Option* option, const std::string& typeName, const CLI::Validator& validator, bool configurable) const {
        return setConfigurable(option, configurable) //
            ->type_name(typeName)
            ->check(validator);

        return option;
    }

} // namespace net::config
