#include "support/TestResult.h"
#include "web/http/CiStringMap.h"
#include "web/http/CookieOptions.h"
#include "web/http/http_utils.h"

#include <cctype>
#include <map>
#include <string>
#include <vector>

namespace {
    bool contains(const std::string& haystack, const std::string& needle) {
        return haystack.find(needle) != std::string::npos;
    }

    bool containsEsc(const std::string& value) {
        return value.find('\033') != std::string::npos;
    }

    std::string stripAllowedSgr(const std::string& value) {
        std::string stripped;
        for (std::size_t i = 0; i < value.size();) {
            if (value.compare(i, 5, "\033[32m") == 0 || value.compare(i, 5, "\033[33m") == 0 || value.compare(i, 5, "\033[34m") == 0 ||
                value.compare(i, 5, "\033[39m") == 0 || value.compare(i, 4, "\033[0m") == 0) {
                i += value[i + 2] == '0' ? 4 : 5;
            } else {
                stripped.push_back(value[i]);
                ++i;
            }
        }
        return stripped;
    }

    void expectPresentation(tests::support::TestResult& result,
                            const std::string& legacy,
                            const httputils::MessagePresentation& presentation,
                            const std::string& context) {
        result.expectTrue(legacy == presentation.plain, context + " plain presentation matches legacy toString");
        result.expectTrue(!containsEsc(presentation.plain), context + " plain presentation contains no raw ESC");
        result.expectTrue(!contains(presentation.plain, "\\x1B["), context + " plain presentation contains no literal color clutter");
        result.expectTrue(contains(presentation.terminal, "\033[34m"), context + " terminal presentation contains offset color");
        result.expectTrue(contains(presentation.terminal, "\033[32m"), context + " terminal presentation contains byte color");
        result.expectTrue(contains(presentation.terminal, "\033[33m"), context + " terminal presentation contains ASCII color");
        result.expectTrue(!contains(presentation.terminal, "\\x1B["), context + " terminal presentation contains no literal color clutter");
        result.expectTrue(stripAllowedSgr(presentation.terminal) == presentation.plain, context + " terminal presentation strips to plain");
    }
} // namespace

int main() {
    tests::support::TestResult result;

    const std::vector<char> requestBody = {'G', 'E', 'T', '\0', '\033', '\n', 'Z'};
    web::http::CiStringMap<std::string> requestHeaders{{"Host", "example.test"}, {"Content-Length", "7"}};
    web::http::CiStringMap<std::string> requestTrailers{{"Digest", "sha-256=abc"}};
    web::http::CiStringMap<std::string> requestCookies{{"session", "abc123"}};
    std::map<std::string, std::string> queries{{"a", "1"}, {"z", "last"}};
    const auto requestPresentation = httputils::toStringPresentation(
        "POST", "/submit?z=last&a=1", "HTTP/1.1", queries, requestHeaders, requestTrailers, requestCookies, requestBody);
    expectPresentation(result,
                       httputils::toString(
                           "POST", "/submit?z=last&a=1", "HTTP/1.1", queries, requestHeaders, requestTrailers, requestCookies, requestBody),
                       requestPresentation,
                       "request with body");
    result.expectTrue(contains(requestPresentation.plain, "Body: 00000000  47 45 54 00 1b 0a 5a                             GET...Z"),
                      "request body preserves offset, byte, ASCII, spacing, and indentation");

    const std::vector<char> responseBody = {'R', 'E', 'S', 'P', '\0', '\033', '\n', 'X', 'Y'};
    web::http::CiStringMap<std::string> responseHeaders{{"Content-Length", "9"}, {"Content-Type", "text/plain"}};
    web::http::CookieOptions cookieOptions("cookie-value");
    cookieOptions.setOption("Path", "/");
    web::http::CiStringMap<web::http::CookieOptions> responseCookies{{"token", cookieOptions}};
    const auto responsePresentation =
        httputils::toStringPresentation("HTTP/1.1", "200", "OK", responseHeaders, responseCookies, responseBody);
    expectPresentation(result,
                       httputils::toString("HTTP/1.1", "200", "OK", responseHeaders, responseCookies, responseBody),
                       responsePresentation,
                       "response with body");
    result.expectTrue(contains(responsePresentation.plain, "Body: 00000000  52 45 53 50 00 1b 0a 58 59                       RESP...XY"),
                      "response partial final row preserves padding and layout");

    const auto emptyRequest = httputils::toStringPresentation("GET", "/", "HTTP/1.1", {}, {}, {}, {}, {});
    result.expectTrue(emptyRequest.plain == emptyRequest.terminal, "empty request presentation is byte-identical");
    result.expectTrue(httputils::toString("GET", "/", "HTTP/1.1", {}, {}, {}, {}, {}) == emptyRequest.plain,
                      "empty request presentation matches legacy toString");

    const auto emptyResponse = httputils::toStringPresentation("HTTP/1.1", "204", "No Content", {}, {}, {});
    result.expectTrue(emptyResponse.plain == emptyResponse.terminal, "empty response presentation is byte-identical");
    result.expectTrue(httputils::toString("HTTP/1.1", "204", "No Content", {}, {}, {}) == emptyResponse.plain,
                      "empty response presentation matches legacy toString");

    return result.processResult();
}
