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

#include "ConfigWWW.h"

#include "net/config/ConfigInstanceAPI.hpp"
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace section {

    ConfigWWW::ConfigWWW(net::config::ConfigInstance* instance)
        : net::config::ConfigSection(instance, this) {
        htmlRootOpt = addOption("--html-root", "HTML root directory", "path", CLI::ExistingDirectory);
        required(htmlRootOpt);
    }

    section::ConfigWWW& section::ConfigWWW::setHtmlRoot(const std::string& htmlRoot) {
        setDefaultValue(htmlRootOpt, htmlRoot);
        required(htmlRootOpt, false);

        return *this;
    }

    std::string ConfigWWW::getHtmlRoot() {
        return htmlRootOpt->as<std::string>();
    }

} // namespace section

namespace instance {

    ConfigWWW::ConfigWWW()
        : utils::SubCommand(
              utils::Config::newInstance(net::config::Instance(std::string(name), std::string(description), this), "Applications", true)) {
        htmlRootOpt = addOption("--html-root", "HTML root directory", "directory", "path", CLI::ExistingDirectory);

        subCommandSc->required()->needs(htmlRootOpt);
        subCommandSc->get_parent()->needs(subCommandSc);
    }

    ConfigWWW& ConfigWWW::setHtmlRoot(const std::string& htmlRoot) {
        setDefaultValue(htmlRootOpt, htmlRoot);
        htmlRootOpt->required(false);

        subCommandSc->required(false)->remove_needs(htmlRootOpt);
        subCommandSc->get_parent()->remove_needs(subCommandSc);

        return *this;
    }

    std::string ConfigWWW::getHtmlRoot() {
        return htmlRootOpt->as<std::string>();
    }

} // namespace instance
