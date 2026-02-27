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

#ifndef NET_CONFIG_CONFIGSECTION_HPP
#define NET_CONFIG_CONFIGSECTION_HPP

#include "net/config/ConfigInstance.h"
#include "net/config/ConfigSection.h" // IWYU pragma: export
#include "utils/ConfigApp.h"          // IWYU pragma: export

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
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <memory>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    template <typename T>
    std::shared_ptr<CLI::App> Section(T* section) {
        return std::make_shared<utils::AppWithPtr<T>>(std::string(T::description), std::string(T::name), section);
    }

    template <typename T>
    ConfigSection::ConfigSection(ConfigInstance* instance, T* sectionPtr, const std::string& group)
        : SubCommand(instance->newSection(net::config::Section(sectionPtr), group))
        , instance(instance) {
        subCommandSc->description(std::string(T::description) + " for instance '" + instance->getInstanceName() + "'");
    }

} // namespace net::config

#endif // NET_CONFIG_CONFIGSECTION_HPP
