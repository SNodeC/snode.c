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
    Request(HTTPContext* httpContext);

    std::multimap<std::string, std::string>& header() const;
    const std::string& header(const std::string& key, int i = 0) const;
    const std::string& cookie(const std::string& key) const;

    const std::string& query(std::string key) const;
    const std::string& httpVersion() const;
    const std::string& fragment() const;

    const std::string& method() const;

    int bodySize() const;

    // Properties
    const std::string& originalUrl;
    mutable std::string url;
    char*& body;
    const std::string& path;

private:
    template <typename Attribute> class AttributeProxy {
    public:
        explicit AttributeProxy()
            : _value(Attribute()) {
        }

        virtual ~AttributeProxy() = default;

        const Attribute& operator=(const Attribute& value) const {
            _value = const_cast<Attribute&>(value);
            return _value;
        }

        Attribute& operator=(Attribute& value) const {
            _value = value;
            return _value;
        }


        operator Attribute&() const {
            return _value;
        }

    private:
        mutable Attribute _value;
    };

public:
    template <typename Attribute> const Attribute getAttribute(const std::string& key = "") const {
        Attribute attribute = Attribute();

        std::map<std::string, std::shared_ptr<void>>::const_iterator it = attributes.find(typeid(Attribute).name() + key);
        if (it != attributes.end()) {
            attribute = static_cast<Attribute>(*std::static_pointer_cast<AttributeProxy<Attribute>>(it->second));
        }

        return attribute;
    }

    template <typename Attribute> bool setAttribute(const Attribute& attribute, const std::string& key = "") const {
        bool inserted = false;

        if (attributes.find(typeid(Attribute).name() + key) == attributes.end()) {
            std::shared_ptr<AttributeProxy<Attribute>> a(new AttributeProxy<Attribute>);
            *a = attribute;
            attributes.insert(
                std::pair<std::string, std::shared_ptr<void>>(typeid(Attribute).name() + key, std::static_pointer_cast<void>(a)));
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
