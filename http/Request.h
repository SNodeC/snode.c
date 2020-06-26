#ifndef REQUEST_H
#define REQUEST_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class HTTPContext;

class Request {
private:
public:
    explicit Request(HTTPContext* httpContext);

    const std::string& header(const std::string& key, int i = 0) const;
    const std::string& cookie(const std::string& key) const;
    const std::string& query(const std::string& key) const;
    const std::string& httpVersion() const;
    const std::string& fragment() const;
    const std::string& method() const;
    int bodySize() const;

    // Properties
    std::string originalUrl{""};
    std::string _httpVersion{""};
    mutable std::string url;
    char*& body;
    std::string path;
    std::map<std::string, std::string> queryMap;
    std::multimap<std::string, std::string> requestHeader;
    std::map<std::string, std::string> requestCookies;

private:
    template <typename Attribute>
    class AttributeProxy {
    public:
        explicit AttributeProxy(const Attribute& attribute)
            : _attribute(attribute) { // copy constructor neccessary
        }

        Attribute& attribute() {
            return _attribute;
        }

    private:
        static_assert(std::is_copy_constructible<Attribute>::value, "Attribute requires copying");
        static_assert(std::is_default_constructible<Attribute>::value, "Attribute requires construction");
        static_assert(std::is_assignable<Attribute&, Attribute>::value, "Attribute requires assignment");

        Attribute _attribute;
    };

public:
    template <typename Attribute>
    Attribute getAttribute(const std::string& key = "") const {
        Attribute attribute = Attribute(); // default constructor & copy constructor neccessary

        std::map<std::string, std::shared_ptr<void>>::const_iterator it = attributes.find(typeid(Attribute).name() + key);
        if (it != attributes.end()) {
            attribute = std::static_pointer_cast<AttributeProxy<Attribute>>(it->second)->attribute(); // assignment operator neccessary
        }

        return attribute;
    }

    template <typename Attribute>
    bool hasAttribute(const std::string& key = "") const {
        return attributes.find(typeid(Attribute).name() + key) != attributes.end();
    }

    template <typename Attribute>
    bool setAttribute(const Attribute& attribute, const std::string& key = "") const {
        bool inserted = false;

        if (attributes.find(typeid(Attribute).name() + key) == attributes.end()) {
            attributes[typeid(Attribute).name() + key] =
                std::static_pointer_cast<void>(std::shared_ptr<AttributeProxy<Attribute>>(new AttributeProxy<Attribute>(attribute)));
            inserted = true;
        }

        return inserted;
    }

private:
    HTTPContext* httpContext;

    mutable std::map<std::string, std::shared_ptr<void>> attributes;

    std::string nullstr = "";
};

#endif // REQUEST_H
