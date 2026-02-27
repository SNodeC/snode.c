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

#include "SubCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    SubCommand::SubCommand(CLI::App* subCommandSc)
        : subCommandSc(subCommandSc) {
    }

    CLI::Option* SubCommand::getOption(const std::string& name) const {
        return subCommandSc->get_option(name);
    }

    CLI::Option* SubCommand::addOption(const std::string& name,
                                       const std::string& description,
                                       const std::string& typeName,
                                       const CLI::Validator& validator) {
        return initialize(subCommandSc //
                              ->add_option(name, description),
                          typeName,
                          validator,
                          !subCommandSc->get_disabled());
    }

    CLI::Option* SubCommand::addOptionFunction(const std::string& name,
                                               std::function<void(const std::string&)>& callback,
                                               const std::string& description,
                                               const std::string& typeName,
                                               const CLI::Validator& validator) {
        return initialize(subCommandSc //
                              ->add_option_function(name, callback, description),
                          typeName,
                          validator,
                          !subCommandSc->get_disabled());
    }

    CLI::Option* SubCommand::addFlag(const std::string& name,
                                     const std::string& description,
                                     const std::string& typeName,
                                     const CLI::Validator& validator) {
        return initialize(subCommandSc //
                              ->add_flag(name, description),
                          typeName,
                          validator,
                          !subCommandSc->get_disabled());
    }

    CLI::Option* SubCommand::addFlagFunction(const std::string& name,
                                             const std::function<void(std::int64_t)>& callback,
                                             const std::string& description,
                                             const std::string& typeName,
                                             const CLI::Validator& validator) {
        return initialize(subCommandSc //
                              ->add_flag_function(name, callback, description),
                          typeName,
                          validator,
                          !subCommandSc->get_disabled());
    }

    CLI::Option* SubCommand::setConfigurable(CLI::Option* option, bool configurable) const {
        option //
            ->configurable(configurable)
            ->group(subCommandSc->get_formatter()->get_label(configurable ? "Persistent Options" : "Nonpersistent Options"));

        return option;
    }

    CLI::Option*
    SubCommand::initialize(CLI::Option* option, const std::string& typeName, const CLI::Validator& validator, bool configurable) const {
        return setConfigurable(option, configurable) //
            ->type_name(typeName)
            ->check(validator);

        return option;
    }

} // namespace utils
