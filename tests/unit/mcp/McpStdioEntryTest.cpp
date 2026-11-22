// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/SNodeC.h"
#include "mcp/server/Config.h"
#include "mcp/server/Server.h"
#include "mcp/server/stream/SocketContextFactory.h"
#include "net/stdio/stream/SocketEndpoint.h"
#include "tests/support/TestResult.h"
#include "utils/Config.h"

#include <cerrno>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <type_traits>
#include <unistd.h>

static_assert(!std::is_copy_constructible_v<mcp::server::Server> && !std::is_move_constructible_v<mcp::server::Server>);
static_assert(!std::is_copy_assignable_v<mcp::server::Server> && !std::is_move_assignable_v<mcp::server::Server>);
static_assert(!std::is_copy_constructible_v<mcp::server::Config> && !std::is_move_constructible_v<mcp::server::Config>);
static_assert(!std::is_copy_assignable_v<mcp::server::Config> && !std::is_move_assignable_v<mcp::server::Config>);
int main(int argc, char* argv[]) {
    tests::support::TestResult test;
    auto* config = utils::Config::configRoot.newSubCommand<mcp::server::Config>();
    mcp::server::Server server("entry", "test-version");
    using Endpoint =
        net::stdio::stream::SocketEndpoint<mcp::server::stream::SocketContextFactory, mcp::server::Server&, mcp::server::Config&>;
    const Endpoint endpoint("entry", server, *config);
    endpoint.getConfig()->Instance::forceUnrequired();
    endpoint.getConfig()->Connection::setReadTimeout(0)->setWriteTimeout(0);
    char option[] = "--config-file=/dev/null", quiet[] = "--quiet";
    char* arguments[] = {argv[0], option, quiet};
    const bool wrongOrder = argc > 1 && std::string(argv[1]) == "wrong-order";
    int input[2]{}, output[2]{}, completions = 0;
    test.expectEqual(0, ::pipe(input), "create a real protocol input pipe");
    test.expectEqual(0, ::pipe(output), "create a real protocol output pipe");
    ::close(input[1]);
    struct stat before{}, after{};
    const int flags = ::fcntl(STDOUT_FILENO, F_GETFL);
    test.expectEqual(0, ::fstat(STDOUT_FILENO, &before), "capture stdout before endpoint activation");
    core::SNodeC::init(3, arguments);
    endpoint.attach(net::stdio::SocketAddress(input[0], output[1]), [&](const auto&, auto state) {
        ++completions;
        test.expectTrue(state == (wrongOrder ? core::socket::State::ERROR : core::socket::State::OK),
                        "entry obeys event-loop activation order");
    });
    test.expectEqual(0, completions, "attachment never activates synchronously");
    if (wrongOrder) {
        static_cast<void>(core::SNodeC::tick());
        test.expectTrue(::fcntl(input[0], F_GETFD) >= 0 && ::fcntl(output[1], F_GETFD) >= 0, "rejected entry retains caller ownership");
        core::SNodeC::free();
        ::close(input[0]);
        ::close(output[1]);
    } else {
        test.expectEqual(0, core::SNodeC::start(), "ordered entry drains EOF successfully");
        test.expectTrue(::fcntl(input[0], F_GETFD) < 0 && ::fcntl(output[1], F_GETFD) < 0, "ordered entry releases owned protocol handles");
    }
    ::close(output[0]);
    test.expectEqual(0, ::fstat(STDOUT_FILENO, &after), "capture stdout after endpoint activation");
    test.expectTrue(before.st_dev == after.st_dev && before.st_ino == after.st_ino && flags == ::fcntl(STDOUT_FILENO, F_GETFL),
                    "the endpoint never redirects stdout or changes its flags");
    test.expectEqual(1, completions, "one completion per executed attempt");
    return test.processResult();
}
