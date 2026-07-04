/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "support/TestResult.h"
#include "web/http/CiStringMap.h"
#include "web/http/CookieOptions.h"
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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
        const std::string formatted = httputils::toString("HTTP/1.1", "200", "OK", {{"Content-Length", "0"}}, {}, {});

        testResult.expectTrue(contains(formatted, "Response"), "response formatter seam identifies response messages");
        testResult.expectTrue(contains(formatted, "Version : HTTP/1.1"), "response formatter seam preserves the HTTP version");
        testResult.expectTrue(contains(formatted, "Status : 200"), "response formatter seam preserves the status code");
        testResult.expectTrue(contains(formatted, "Reason : OK"), "response formatter seam preserves the reason phrase");
        testResult.expectTrue(contains(formatted, "Content-Length : 0"), "response formatter seam preserves stable headers");
    }

    {
        const std::vector<char> body = {'n', 'o', 't', ' ', 'f', 'o', 'u', 'n', 'd'};
        const std::string formatted = httputils::toString("HTTP/1.1",
                                                         "404",
                                                         "Not Found",
                                                         {{"Content-Length", "9"}, {"Content-Type", "text/plain"}},
                                                         {},
                                                         body);

        testResult.expectTrue(contains(formatted, "Status : 404"), "response formatter seam preserves non-200 status codes");
        testResult.expectTrue(contains(formatted, "Reason : Not Found"), "response formatter seam preserves non-200 reason phrases");
        testResult.expectTrue(contains(formatted, "Content-Length : 9"), "response formatter seam preserves response Content-Length");
        testResult.expectTrue(contains(formatted, "Content-Type : text/plain"), "response formatter seam preserves response Content-Type");
        testResult.expectTrue(contains(formatted, "6e 6f 74 20 66 6f 75 6e 64"), "response formatter seam preserves the body bytes");
    }

    return testResult.processResult();
}
