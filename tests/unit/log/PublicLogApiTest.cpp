#include "Log.h"
#include "tests/support/TestResult.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace {
    template <class Function>
    bool throwsInvalidArgument(Function&& function) {
        try {
            std::forward<Function>(function)();
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    }
} // namespace

int main() {
    tests::support::TestResult result;

    result.expectTrue(snode::log::detail::format("received {} bytes", 42) == "received 42 bytes",
                      "public formatter replaces placeholders");
    result.expectTrue(snode::log::detail::format("{{{}}}", "value") == "{value}",
                      "public formatter handles escaped braces");
    result.expectTrue(throwsInvalidArgument([] { snode::log::detail::format("{} {}"); }),
                      "public formatter rejects missing arguments");
    result.expectTrue(throwsInvalidArgument([] { snode::log::detail::format("literal", 1); }),
                      "public formatter rejects excess arguments");
    result.expectTrue(throwsInvalidArgument([] { snode::log::detail::format("{"); }),
                      "public formatter rejects unmatched opening braces");
    result.expectTrue(throwsInvalidArgument([] { snode::log::detail::format("}"); }),
                      "public formatter rejects unmatched closing braces");

    snode::log::Scope scope{.origin = snode::log::Origin::Framework,
                            .boundary = snode::log::Boundary::Connection,
                            .component = "core.socket.stream",
                            .identity = {.instance = "listener",
                                         .role = snode::log::Role::Server,
                                         .connection = "#7"}};
    snode::log::Scope copy = scope;
    scope.component.clear();
    scope.identity.instance->clear();
    result.expectTrue(copy.component == "core.socket.stream", "public scopes own component identity");
    result.expectTrue(copy.identity.instance && *copy.identity.instance == "listener", "public scopes own instance identity");

    return result.processResult();
}
