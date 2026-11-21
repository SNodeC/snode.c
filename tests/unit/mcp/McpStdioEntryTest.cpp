// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/SNodeC.h"
#include "mcp/server/Config.h"
#include "mcp/server/Server.h"
#include "mcp/server/stdio/Transport.h"
#include "tests/support/TestResult.h"
#include "utils/Config.h"
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
    char option[] = "--config-file", path[] = "/dev/null";
    char* arguments[] = {argv[0], option, path, nullptr};
    int ready = 0;
    const bool wrongOrder = argc > 1 && std::string(argv[1]) == "wrong-order";
    if (wrongOrder) {
        core::SNodeC::init(3, arguments);
    } else {
        int input[2] = {-1, -1};
        test.expectEqual(0, ::pipe(input), "create a real protocol input pipe");
        test.expectTrue(::dup2(input[0], STDIN_FILENO) >= 0, "attach the protocol input pipe");
        ::close(input[0]);
        ::close(input[1]);
        test.expectEqual(0,
                         mcp::server::stdio::Transport::run(3,
                                                            arguments,
                                                            server,
                                                            *config,
                                                            [&] {
                                                                ++ready;
                                                            }),
                         "ordered entry initializes and drains EOF successfully");
    }
    struct stat before{}, after{};
    const int flags = ::fcntl(STDOUT_FILENO, F_GETFL);
    test.expectEqual(0, ::fstat(STDOUT_FILENO, &before), "capture stdout before rejected entry");
    test.expectEqual(1,
                     mcp::server::stdio::Transport::run(3,
                                                        arguments,
                                                        server,
                                                        *config,
                                                        [&] {
                                                            ++ready;
                                                        }),
                     "wrong-order or duplicate entry rejected");
    test.expectEqual(0, ::fstat(STDOUT_FILENO, &after), "capture stdout after rejected entry");
    test.expectTrue(before.st_dev == after.st_dev && before.st_ino == after.st_ino && flags == ::fcntl(STDOUT_FILENO, F_GETFL),
                    "rejected entry never redirects stdout or changes its flags");
    test.expectEqual(wrongOrder ? 0 : 1, ready, "Ready runs exactly once, only on accepted entry");
    if (wrongOrder) {
        core::SNodeC::free();
    }
    return test.processResult();
}
