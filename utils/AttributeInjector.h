#ifndef ATTRIBUTEINJECTOR_H
#define ATTRIBUTEINJECTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <map>
#include <memory>

namespace std {

    template <typename CharT, std::size_t N>
    struct basic_fixed_string {
        constexpr basic_fixed_string(const CharT (&foo)[N]) {
            std::copy(std::begin(foo), std::end(foo), std::begin(m_data));
        }

        constexpr auto operator<=>(const basic_fixed_string&) const = default;

        CharT m_data[N]{};

        constexpr operator const CharT*() const {
            return m_data;
        }

        using CharType = CharT;
    };

    template <typename CharT, std::size_t N>
    basic_fixed_string(const CharT (&str)[N]) -> basic_fixed_string<CharT, N>;

    template <std::size_t N>
    using fixed_string = basic_fixed_string<char, N>;

} // namespace std

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    template <typename Attribute>
    concept InjectedAttribute = std::copy_constructible<Attribute>and std::default_initializable<Attribute>and std::copyable<Attribute>;

    template <InjectedAttribute Attribute>
    class AttributeProxy {
    public:
        explicit constexpr AttributeProxy(const Attribute& attribute)
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
        template <InjectedAttribute Attribute>
        constexpr bool setAttribute(Attribute& attribute, bool overwrite = false) const {
            bool inserted = false;

            if (!this->attribute || overwrite) {
                attributeType = typeid(Attribute).name();
                this->attribute = std::shared_ptr<void>(new AttributeProxy<Attribute>(attribute));
                inserted = true;
            }

            return inserted;
        }

        template <InjectedAttribute Attribute>
        constexpr bool setAttribute(Attribute&& attribute, bool overwrite = false) const {
            bool inserted = false;

            if (!this->attribute || overwrite) {
                attributeType = typeid(Attribute).name();
                this->attribute = std::shared_ptr<void>(new AttributeProxy<Attribute>(attribute));
                inserted = true;
            }

            return inserted;
        }

        template <InjectedAttribute Attribute>
        constexpr bool getAttribute(const std::function<void(Attribute& attribute)>& onFound) const {
            bool found = false;

            if (attribute != nullptr && attributeType == typeid(Attribute).name()) {
                onFound(**std::static_pointer_cast<AttributeProxy<Attribute>>(attribute));
            }

            return found;
        }

        template <InjectedAttribute Attribute>
        constexpr void getAttribute(const std::function<void(Attribute& attribute)>& onFound,
                                    const std::function<void(const std::string&)>& onNotFound) const {
            if (attribute != nullptr && attributeType == typeid(Attribute).name()) {
                onFound(**std::static_pointer_cast<AttributeProxy<Attribute>>(attribute));
            } else {
                onNotFound(std::string(typeid(Attribute).name()));
            }
        }

    private:
        mutable std::shared_ptr<void> attribute{nullptr};
        mutable std::string attributeType;
    };

    class MultibleAttributeInjector {
    public:
        template <InjectedAttribute Attribute, std::basic_fixed_string key = "">
        constexpr bool setAttribute(Attribute& attribute, const std::string& subKey = "", bool overwrite = false) const {
            bool inserted = false;

            if (attributes.find(typeid(Attribute).name() + std::string(key) + subKey) == attributes.end() || overwrite) {
                attributes[typeid(Attribute).name() + std::string(key) + subKey] =
                    std::shared_ptr<void>(new AttributeProxy<Attribute>(attribute));
                inserted = true;
            }

            return inserted;
        }

        template <InjectedAttribute Attribute, std::basic_fixed_string key = "">
        constexpr bool setAttribute(Attribute&& attribute, const std::string& subKey = "", bool overwrite = false) const {
            bool inserted = false;

            if (attributes.find(typeid(Attribute).name() + std::string(key) + subKey) == attributes.end() || overwrite) {
                attributes[typeid(Attribute).name() + std::string(key) + subKey] =
                    std::shared_ptr<void>(new AttributeProxy<Attribute>(attribute));
                inserted = true;
            }

            return inserted;
        }

        template <InjectedAttribute Attribute, std::basic_fixed_string key = "">
        constexpr bool delAttribute(const std::string& subKey = "") const {
            return attributes.erase(typeid(Attribute).name() + std::string(key) + subKey) > 0;
        }

        template <InjectedAttribute Attribute, std::basic_fixed_string key = "">
        constexpr bool hasAttribute(const std::string& subKey = "") const {
            return attributes.find(typeid(Attribute).name() + std::string(key) + subKey) != attributes.end();
        }

        template <InjectedAttribute Attribute, std::basic_fixed_string key = "">
        constexpr bool getAttribute(const std::function<void(Attribute&)>& onFound, const std::string& subKey = "") const {
            bool found = false;

            if ((found = hasAttribute<Attribute, key>(subKey))) {
                std::map<std::string, std::shared_ptr<void>>::const_iterator it =
                    attributes.find(typeid(Attribute).name() + std::string(key) + subKey);
                onFound(**std::static_pointer_cast<AttributeProxy<Attribute>>(it->second));
            }

            return found;
        }

        template <InjectedAttribute Attribute, std::basic_fixed_string key = "">
        constexpr void getAttribute(const std::function<void(Attribute&)>& onFound,
                                    const std::function<void(const std::string&)>& onNotFound, const std::string& subKey = "") const {
            if (hasAttribute<Attribute, key>(subKey)) {
                std::map<std::string, std::shared_ptr<void>>::const_iterator it =
                    attributes.find(typeid(Attribute).name() + std::string(key) + subKey);
                onFound(**std::static_pointer_cast<AttributeProxy<Attribute>>(it->second));
            } else {
                onNotFound(std::string(typeid(Attribute).name()) + std::string(key) + subKey);
            }
        }

        void reset() {
            attributes.clear();
        }

    private:
        mutable std::map<std::string, std::shared_ptr<void>> attributes;
    };

} // namespace utils

#endif // ATTRIBUTEINJECTOR_H
