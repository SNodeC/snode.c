//
// Created by student on 5/24/20.
//

#ifndef SNODE_C_COOKIE_H
#define SNODE_C_COOKIE_H

#include <map>
#include <string>

class Cookie {
public:
    Cookie(const std::string& value, const std::map<std::string, std::string>& options) : value(value), options(options) {}
    Cookie() {}
    Cookie(const std::string& name, const std::string& value) : name(name), value(value) {}
    void Domain(const std::string& domain);
    void Path(const std::string& path);
    void MaxAge(const std::string& maxAge);
    void Expires(const std::string& expires);
    void MakeEssential();
    void SetSameSite(const bool strict);
    void MakeSecure();
    void MakeHttpOnly();
    void reset();

    const std::string &getName() const;
    const std::string &getValue() const;
    const std::map<std::string, std::string> &getOptions() const;

protected:
    std::string name;
    std::string value;
    std::map<std::string, std::string> options;


protected:

    friend class HTTPContext;
};


#endif //SNODE_C_COOKIE_H
