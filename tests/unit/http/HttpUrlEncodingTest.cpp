/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "support/TestResult.h"
#include "web/http/http_utils.h"

#include <string>

int main() {
    tests::support::TestResult testResult;

    testResult.expectTrue(httputils::url_encode("v1.0.0+build") == "v1.0.0%2Bbuild", "encode semantic-version build metadata");
    testResult.expectTrue(httputils::url_encode("").empty(), "preserve empty input");
    testResult.expectTrue(httputils::url_encode("AZaz09-_.~") == "AZaz09-_.~", "preserve ASCII unreserved characters");
    testResult.expectTrue(httputils::url_encode(" /?&=+#%") == "%20%2F%3F%26%3D%2B%23%25", "encode reserved characters");
    testResult.expectTrue(httputils::url_encode(std::string("\0\x01\x0F\x7F\x80\xFF", 6)) == "%00%01%0F%7F%80%FF",
                          "encode unsigned bytes using two uppercase hexadecimal digits");
    testResult.expectTrue(httputils::url_encode("\xC3\x9F") == "%C3%9F", "encode each UTF-8 byte");

    return testResult.processResult();
}
