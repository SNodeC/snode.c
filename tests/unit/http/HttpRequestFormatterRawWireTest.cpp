/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "support/TestResult.h"
#include "web/http/CiStringMap.h"
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    bool contains(const std::string& haystack, const std::string& needle) {
        return haystack.find(needle) != std::string::npos;
    }

} // namespace

int main() {
    tests::support::TestResult testResult;

    {
        const std::string formatted = httputils::toString("GET",
                                                         "/index.html",
                                                         "HTTP/1.1",
                                                         {},
                                                         {{"Host", "example.test"}},
                                                         {},
                                                         {},
                                                         {});

        testResult.expectTrue(contains(formatted, "Request"), "request formatter seam identifies request messages");
        testResult.expectTrue(contains(formatted, "Method : GET"), "request formatter seam preserves the GET method");
        testResult.expectTrue(contains(formatted, "Url : /index.html"), "request formatter seam preserves the request target");
        testResult.expectTrue(contains(formatted, "Version : HTTP/1.1"), "request formatter seam preserves the HTTP version");
        testResult.expectTrue(contains(formatted, "Host : example.test"), "request formatter seam preserves stable headers");
    }

    {
        const std::vector<char> body = {'h', 'e', 'l', 'l', 'o'};
        const std::string formatted = httputils::toString("POST",
                                                         "/submit",
                                                         "HTTP/1.1",
                                                         {},
                                                         {{"Content-Length", "5"}, {"Content-Type", "text/plain"}, {"Host", "example.test"}},
                                                         {},
                                                         {},
                                                         body);

        testResult.expectTrue(contains(formatted, "Method : POST"), "request formatter seam preserves the POST method");
        testResult.expectTrue(contains(formatted, "Url : /submit"), "request formatter seam preserves the POST target");
        testResult.expectTrue(contains(formatted, "Content-Length : 5"), "request formatter seam preserves Content-Length");
        testResult.expectTrue(contains(formatted, "Content-Type : text/plain"), "request formatter seam preserves Content-Type");
        testResult.expectTrue(contains(formatted, "Host : example.test"), "request formatter seam preserves Host");
        testResult.expectTrue(contains(formatted, "68 65 6c 6c 6f"), "request formatter seam preserves the body bytes");
    }

    return testResult.processResult();
}
