#ifndef REQUEST_H
#define REQUEST_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <memory>
#include <string>

// remove this template as soon as g++/clang supports it natively
namespace std {
    template <typename CharT, std::size_t N>
    struct basic_fixed_string {
        constexpr basic_fixed_string(const CharT (&foo)[N]) {
            std::copy(std::begin(foo), std::end(foo), std::begin(m_data));
        }

        auto operator<=>(const basic_fixed_string&) const = default;

        CharT m_data[N]{};

        constexpr operator const CharT*() const {
            return m_data;
        }
    };
    template <typename CharT, std::size_t N>
    basic_fixed_string(const CharT (&str)[N]) -> basic_fixed_string<CharT, N>;

    template <std::size_t N>
    using fixed_string = basic_fixed_string<char, N>;
} // namespace std

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class HTTPContext;

class Request {
private:
    explicit Request(HTTPContext* httpContext);

public:
    const std::string& header(const std::string& key, int i = 0) const;
    const std::string& cookie(const std::string& key) const;
    const std::string& query(const std::string& key) const;
    const std::string& fragment() const;
    int bodySize() const;

    // Properties
    std::string originalUrl{""};
    std::string httpVersion{""};
    mutable std::string url;
    char* body{nullptr};
    std::string path;
    std::map<std::string, std::string> queryMap;
    std::multimap<std::string, std::string> requestHeader;
    std::map<std::string, std::string> requestCookies;
    std::string method{""};
    int bodyLength{0};

private:
    template <typename Attribute>
    class AttributeProxy {
    public:
        explicit AttributeProxy(const Attribute& attribute)
            : attribute(attribute) { // copy constructor neccessary
        }

        Attribute& getAttribute() {
            return attribute;
        }

    private:
        static_assert(std::is_copy_constructible<Attribute>::value, "Attribute requires capability \"copy_constructible\"");
        static_assert(std::is_default_constructible<Attribute>::value, "Attribute requires capability \"default_constructible\"");
        static_assert(std::is_assignable<Attribute&, Attribute>::value, "Attribute requires capability \"assignable\"");

        Attribute attribute;
    };

public:
    template <typename Attribute, std::basic_fixed_string key = "">
    bool setAttribute(const Attribute& attribute, bool overwrite = false) const {
        bool inserted = false;

        if (attributes.find(typeid(Attribute).name() + std::string(key)) == attributes.end() || overwrite) {
            attributes[typeid(Attribute).name() + std::string(key)] =
                std::static_pointer_cast<void>(std::shared_ptr<AttributeProxy<Attribute>>(new AttributeProxy<Attribute>(attribute)));
            inserted = true;
        }

        return inserted;
    }

    template <typename Attribute, std::basic_fixed_string key = "">
    bool hasAttribute() const {
        return attributes.find(typeid(Attribute).name() + std::string(key)) != attributes.end();
    }

    template <typename Attribute, std::basic_fixed_string key = "">
    Attribute getAttribute() const {
        Attribute attribute = Attribute(); // default constructor & copy constructor neccessary

        std::map<std::string, std::shared_ptr<void>>::const_iterator it = attributes.find(typeid(Attribute).name() + std::string(key));
        if (it != attributes.end()) {
            attribute = std::static_pointer_cast<AttributeProxy<Attribute>>(it->second)->getAttribute(); // assignment operator neccessary
        }

        return attribute;
    }

    template <typename Attribute, std::basic_fixed_string key = "">
    bool getAttribute(std::function<void(Attribute& attribute)> onFound) const {
        bool found = false;

        if ((found = hasAttribute<Attribute, key>())) {
            std::map<std::string, std::shared_ptr<void>>::const_iterator it = attributes.find(typeid(Attribute).name() + std::string(key));
            onFound(std::static_pointer_cast<AttributeProxy<Attribute>>(it->second)->getAttribute());
        }

        return found;
    }

    template <typename Attribute, std::basic_fixed_string key = "">
    void getAttribute(std::function<void(Attribute& attribute)> onFound, std::function<void(const std::string&)> onNotFound) const {
        if (hasAttribute<Attribute, key>()) {
            std::map<std::string, std::shared_ptr<void>>::const_iterator it = attributes.find(typeid(Attribute).name() + std::string(key));
            onFound(std::static_pointer_cast<AttributeProxy<Attribute>>(it->second)->getAttribute());
        } else {
            onNotFound(std::string(typeid(Attribute).name()) + std::string(key));
        }
    }

private:
    void reset();

    HTTPContext* httpContext;

    mutable std::map<std::string, std::shared_ptr<void>> attributes;

    std::string nullstr = "";

    friend class HTTPContext;
};

#endif // REQUEST_H
