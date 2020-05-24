//
// Created by student on 5/24/20.
//

#include "Cookie.h"


void Cookie::Domain(const std::string &domain) {
    this->options.insert({ "Domain", domain});
}

void Cookie::Path(const std::string &path) {
    this->options.insert({ "Path", path});
}

void Cookie::MaxAge(const std::string &maxAge) {
    this->options.insert({ "Max-Age", maxAge});
}

void Cookie::Expires(const std::string &expires) {
    this->options.insert({ "Expires", expires});
}

void Cookie::MakeEssential() {
    this->options.insert({ "IsEssential", "true"});
}

void Cookie::MakeSecure() {
    this->options.insert({ "Secure", "true"});
}

void Cookie::MakeHttpOnly() {
    this->options.insert({ "HttpOnly", "true"});
}

void Cookie::SetSameSite(const bool strict) {
    this->options.insert( { "SameSite", strict ? "Strict" : "None" });
}

void Cookie::reset() {
    this->value = "";
    this->options.clear();
}

const std::string &Cookie::getName() const {
    return name;
}

const std::string &Cookie::getValue() const {
    return value;
}

const std::map<std::string, std::string> &Cookie::getOptions() const {
    return options;
}