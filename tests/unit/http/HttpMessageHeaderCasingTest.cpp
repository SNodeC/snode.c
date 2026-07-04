/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "support/TestResult.h"
#include "web/http/CiStringMap.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main() {
    tests::support::TestResult testResult;

    {
        web::http::CiStringMap<std::string> headers;
        headers.emplace("Content-Type", "text/plain");

        testResult.expectTrue(headers.find("content-type") != headers.end(), "headers are found with lower-case lookup names");
        testResult.expectTrue(headers.find("CONTENT-TYPE") != headers.end(), "headers are found with upper-case lookup names");
        testResult.expectTrue(headers.find("Content-Type") != headers.end(), "headers are found with original-case lookup names");
        testResult.expectTrue(headers["content-type"] == "text/plain", "lower-case lookup returns the stored header value");
        testResult.expectTrue(headers["CONTENT-TYPE"] == "text/plain", "upper-case lookup returns the stored header value");
    }

    {
        web::http::CiStringMap<std::string> headers;
        headers.emplace("X-Trace", "one");
        headers.insert_or_assign("x-trace", "two");

        testResult.expectEqual(1, static_cast<int>(headers.size()), "duplicate non-cookie header names with different casing share one map entry");
        testResult.expectTrue(headers["X-TRACE"] == "two", "insert_or_assign replaces duplicate non-cookie header values case-insensitively");
        testResult.expectTrue(headers.begin()->first == "X-Trace", "original casing from the first inserted header name is preserved by the map key");
    }

    {
        web::http::CiStringMap<std::string> headers;
        headers.insert_or_assign("Set-Cookie", "a=1");
        headers.insert_or_assign("set-cookie", "b=2");

        testResult.expectEqual(1, static_cast<int>(headers.size()), "Set-Cookie lookup uses the same case-insensitive header map contract");
        testResult.expectTrue(headers["SET-COOKIE"] == "b=2", "Set-Cookie replacement is case-insensitive in the header map");
    }

    return testResult.processResult();
}
