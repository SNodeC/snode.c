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

#ifndef NET_CONFIG_CONFIGINSTANCE_HPP
#define NET_CONFIG_CONFIGINSTANCE_HPP

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "net/config/ConfigInstance.h" // IWYU pragma: export
#include "utils/Config.h"              // IWYU pragma: export
#include "utils/ConfigApp.h"           // IWYU pragma: export

#include <cstdint>
#include <memory> // IWYU pragma: export
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    template <typename T>
    std::shared_ptr<utils::AppWithPtr<T>> Instance(const std::string& name, const std::string& description, T* section) {
        return std::make_shared<utils::AppWithPtr<T>>(description, name, section);
    }

    template <typename SectionType>
    SectionType* ConfigInstance::getSection(bool onlyGot, bool recursive) const {
        utils::AppWithPtr<SectionType>* resultApp = nullptr;

        auto* appWithPtr = instanceSc->get_subcommand_no_throw(std::string(SectionType::name));

        utils::AppWithPtr<SectionType>* sectionApp = dynamic_cast<utils::AppWithPtr<SectionType>*>(appWithPtr);

        if (sectionApp != nullptr) {
            utils::AppWithPtr<SectionType>* parentApp = dynamic_cast<utils::AppWithPtr<SectionType>*>(
                sectionApp->get_parent()->get_subcommand_no_throw(std::string(SectionType::name)));

            if (sectionApp->count_all() > 0 || !onlyGot) {
                resultApp = sectionApp;
            } else if (recursive && parentApp != nullptr && (parentApp->count_all() > 0 || !onlyGot)) {
                resultApp = parentApp;
            }
        }

        return resultApp != nullptr ? resultApp->getPtr() : nullptr;
    }

    template <typename ConcreteConfigSection, typename... Args>
    void ConfigInstance::addSection(Args&&... args) {
        addSection(std::make_shared<ConcreteConfigSection>(this, std::forward<Args>(args)...));
    }

} // namespace net::config

namespace utils {

    template <typename T>
    T* Config::getInstance() {
        auto* appWithPtr = app->get_subcommand_no_throw(std::string(T::name));

        AppWithPtr<T>* instanceApp = dynamic_cast<utils::AppWithPtr<T>*>(appWithPtr);

        return instanceApp != nullptr ? instanceApp->getPtr() : nullptr;
    }

    template <typename T>
    T* Config::addInstance() {
        return std::static_pointer_cast<T>(configSections.emplace_back(std::make_shared<T>())).get();
    }

} // namespace utils

#endif // NET_CONFIG_CONFIGINSTANCE_HPP
