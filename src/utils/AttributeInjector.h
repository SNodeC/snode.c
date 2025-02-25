/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef ATTRIBUTEINJECTOR_H
#define ATTRIBUTEINJECTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <functional>
#include <map>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    template <unsigned N>
    struct fixed_string {
        char buf[N + 1]{};
        constexpr fixed_string(char const* s) { // cppcheck-suppress noExplicitConstructor
            for (unsigned i = 0; i != N; ++i) {
                buf[i] = s[i];
            }
        }

        constexpr operator char const*() const {
            return buf;
        }

        auto operator<=>(const fixed_string&) const = default;
    };
    template <unsigned N>
    fixed_string(char const (&)[N]) -> fixed_string<N - 1>;

    template <typename Attribute>
    concept InjectableAttribute = std::copy_constructible<Attribute> and std::default_initializable<Attribute> and std::copyable<Attribute>;

    template <InjectableAttribute Attribute>
    class AttributeProxy {
    public:
        constexpr explicit AttributeProxy(const Attribute& attribute)
            : attribute(attribute) { // copy constructor neccessary
        }

        constexpr Attribute& operator*() {
            return attribute;
        }

    private:
        Attribute attribute;
    };

    class SingleAttributeInjector {
    public:
        template <InjectableAttribute Attribute>
        constexpr bool setAttribute(const Attribute& attribute, bool overwrite = false) {
            bool inserted = false;

            if (!this->attribute || overwrite) {
                attributeType = typeid(Attribute).name();
                this->attribute = std::shared_ptr<void>(new AttributeProxy<Attribute>(attribute));
                inserted = true;
            }

            return inserted;
        }

        template <InjectableAttribute Attribute>
        constexpr bool setAttribute(const Attribute&& attribute, bool overwrite = false) {
            bool inserted = false;

            if (!this->attribute || overwrite) {
                attributeType = typeid(Attribute).name();
                this->attribute = std::shared_ptr<void>(new AttributeProxy<Attribute>(attribute));
                inserted = true;
            }

            return inserted;
        }

        template <InjectableAttribute Attribute>
        constexpr bool getAttribute(const std::function<void(Attribute&)>& onFound) const {
            const bool found = false;

            if (attribute != nullptr && attributeType == typeid(Attribute).name()) {
                onFound(**std::static_pointer_cast<AttributeProxy<Attribute>>(attribute));
            }

            return found;
        }

        template <InjectableAttribute Attribute>
        constexpr void getAttribute(const std::function<void(Attribute&)>& onFound,
                                    const std::function<void(const std::string&)>& onNotFound) const {
            if (attribute != nullptr && attributeType == typeid(Attribute).name()) {
                onFound(**std::static_pointer_cast<AttributeProxy<Attribute>>(attribute));
            } else {
                onNotFound(std::string(typeid(Attribute).name()));
            }
        }

    private:
        std::shared_ptr<void> attribute{nullptr};
        std::string attributeType;
    };

    class MultibleAttributeInjector {
    public:
        template <InjectableAttribute Attribute, fixed_string key = "">
        constexpr bool setAttribute(const Attribute& attribute, const std::string& subKey = "", bool overwrite = false) {
            bool inserted = false;

            if (attributes.find(typeid(Attribute).name() + std::string(key) + subKey) == attributes.end() || overwrite) {
                attributes[typeid(Attribute).name() + std::string(key) + subKey] =
                    std::shared_ptr<void>(new AttributeProxy<Attribute>(attribute));
                inserted = true;
            }

            return inserted;
        }

        template <InjectableAttribute Attribute, fixed_string key = "">
        constexpr bool setAttribute(const Attribute&& attribute, const std::string& subKey = "", bool overwrite = false) {
            bool inserted = false;

            if (attributes.find(typeid(Attribute).name() + std::string(key) + subKey) == attributes.end() || overwrite) {
                attributes[typeid(Attribute).name() + std::string(key) + subKey] =
                    std::shared_ptr<void>(new AttributeProxy<Attribute>(attribute));
                inserted = true;
            }

            return inserted;
        }

        template <InjectableAttribute Attribute, fixed_string key = "">
        constexpr bool delAttribute(const std::string& subKey = "") {
            bool deleted = attributes.erase(typeid(Attribute).name() + std::string(key) + subKey) > 0;

            return deleted;
        }

        template <InjectableAttribute Attribute, fixed_string key = "">
        constexpr bool hasAttribute(const std::string& subKey = "") const {
            bool found = attributes.find(typeid(Attribute).name() + std::string(key) + subKey) != attributes.end();

            return found;
        }

        template <InjectableAttribute Attribute, fixed_string key = "">
        bool getAttribute(const std::function<void(Attribute&)>& onFound, const std::string& subKey = "") const {
            bool found = false;

            std::map<std::string, std::shared_ptr<void>>::const_iterator it =
                attributes.find(typeid(Attribute).name() + std::string(key) + subKey);

            if (it != attributes.end()) {
                found = true;

                onFound(**std::static_pointer_cast<AttributeProxy<Attribute>>(it->second));
            }

            return found;
        }

        template <InjectableAttribute Attribute, fixed_string key = "">
        void getAttribute(const std::function<void(Attribute&)>& onFound,
                          const std::function<void(const std::string&)>& onNotFound,
                          const std::string& subKey = "") const {
            std::map<std::string, std::shared_ptr<void>>::const_iterator it =
                attributes.find(typeid(Attribute).name() + std::string(key) + subKey);

            if (it != attributes.end()) {
                onFound(**std::static_pointer_cast<AttributeProxy<Attribute>>(it->second));
            } else {
                onNotFound(std::string(typeid(Attribute).name()) + std::string(key) + subKey);
            }
        }

        void reset() {
            attributes.clear();
        }

    private:
        std::map<std::string, std::shared_ptr<void>> attributes;
    };

} // namespace utils

#endif // ATTRIBUTEINJECTOR_H
