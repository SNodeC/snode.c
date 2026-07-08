#include "SourcePolicyTestRoot.h"

#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>

namespace {

    bool ordered(std::string_view source, std::initializer_list<std::string_view> fragments) {
        std::size_t pos = 0;
        for (const std::string_view fragment : fragments) {
            pos = source.find(fragment, pos);
            if (pos == std::string_view::npos) {
                return false;
            }
            pos += fragment.size();
        }
        return true;
    }

    bool requireContains(std::string_view source, std::string_view fragment, std::string_view description) {
        if (source_policy::contains(source, fragment)) {
            return true;
        }

        std::cerr << "Missing broader syscall discipline evidence: " << description << "\nExpected fragment: " << fragment << '\n';
        return false;
    }

    bool requireOrdered(std::string_view source, std::initializer_list<std::string_view> fragments, std::string_view description) {
        if (ordered(source, fragments)) {
            return true;
        }

        std::cerr << "Missing broader syscall discipline ordering evidence: " << description << '\n';
        return false;
    }

} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }
    const std::string epollMux =
        source_policy::readSourcePolicyFile(root / "src" / "core" / "multiplexer" / "epoll" / "EventMultiplexer.cpp");
    const std::string eventLoop = source_policy::readSourcePolicyFile(root / "src" / "core" / "EventLoop.cpp");
    const std::string innerEpoll =
        source_policy::readSourcePolicyFile(root / "src" / "core" / "multiplexer" / "epoll" / "DescriptorEventPublisher.cpp");
    const std::string pollMux =
        source_policy::readSourcePolicyFile(root / "src" / "core" / "multiplexer" / "poll" / "EventMultiplexer.cpp");
    const std::string selectMux =
        source_policy::readSourcePolicyFile(root / "src" / "core" / "multiplexer" / "select" / "EventMultiplexer.cpp");
    const std::string epollWrapper = source_policy::readSourcePolicyFile(root / "src" / "core" / "system" / "epoll.cpp");
    const std::string pollWrapper = source_policy::readSourcePolicyFile(root / "src" / "core" / "system" / "poll.cpp");
    const std::string selectWrapper = source_policy::readSourcePolicyFile(root / "src" / "core" / "system" / "select.cpp");
    const std::string coreMux = source_policy::readSourcePolicyFile(root / "src" / "core" / "EventMultiplexer.cpp");

    bool ok = true;
    ok &= requireContains(epollMux, ", epfd(core::system::epoll_create1(EPOLL_CLOEXEC))", "outer epoll_create1 remains in the initializer");
    ok &= requireOrdered(epollMux,
                         {"epfd(core::system::epoll_create1(EPOLL_CLOEXEC)) {",
                          "if (epfd < 0)",
                          "const int errnum = errno;",
                          "Core::multiplexer epoll_create1 failed",
                          "return;"},
                         "outer epoll_create1 is checked first and skips follow-on ctl calls");
    ok &= requireContains(epollMux, "core::system::epoll_ctl(epfd, EPOLL_CTL_ADD", "outer epoll_ctl ADD calls remain explicit");
    ok &= requireContains(epollMux,
                          "Core::multiplexer epoll_ctl ADD failed: fd={} publisher={}",
                          "outer epoll_ctl ADD failures are logged with fd and publisher");
    ok &= requireOrdered(
        epollMux,
        {"bool setupSucceeded = true;", "setupSucceeded &= addPublisherToOuterEpoll", "if (setupSucceeded)", "Core::multiplexer: epoll"},
        "outer epoll success-looking debug line is gated on setup success");

    ok &= requireContains(eventLoop, "logSignalFailure(sigemptyset", "sigemptyset return values are checked");
    ok &= requireContains(eventLoop, "logSigaddsetFailure(sigaddset", "sigaddset return values are checked");
    ok &= requireContains(eventLoop, "logSigactionFailure(sigaction", "sigaction return values are checked");
    ok &= requireContains(eventLoop, "logSignalFailure(sigprocmask", "sigprocmask return values are checked");
    ok &= requireContains(eventLoop, "Core::EventLoop sigaction failed", "sigaction failures use EventLoop sysError diagnostics");
    ok &= requireContains(eventLoop, "logSignalFailure(sigprocmask", "sigprocmask failures use checked EventLoop diagnostics");
    ok &= requireContains(
        eventLoop, "Core::EventLoop {} failed: phase={}", "generic signal syscall failures use EventLoop sysError diagnostics");
    ok &= requireOrdered(
        eventLoop,
        {"SIGPIPE, &oldPipeAct", "SIGTERM, &oldTermAct", "SIGALRM, &oldAlarmAct", "SIGHUP, &oldHupAct", "free();", "SIGINT, &oldIntAct"},
        "start restore ordering, including SIGINT restore-last behavior, is preserved");

    ok &= requireContains(innerEpoll, "core.mux epoll_create1 failed", "inner epoll PR #171 create diagnostics remain present");
    ok &= requireContains(innerEpoll, "core.mux epoll_ctl ADD failed: fd={}", "inner epoll ADD diagnostics remain present");
    ok &= requireContains(innerEpoll, "core.mux epoll_wait failed: interestCount={}", "inner epoll wait diagnostics remain present");
    ok &= requireContains(innerEpoll, "errnum == EEXIST", "inner epoll EEXIST behavior remains present");
    ok &= requireContains(innerEpoll, "errnum == EBADF", "inner epoll EBADF behavior remains present");
    ok &= requireContains(innerEpoll, "if (interestCount > 0)", "inner epoll interestCount underflow guard remains present");

    ok &= requireContains(pollMux, "return core::system::ppoll", "poll outer monitor path directly returns ppoll");
    ok &= requireContains(selectMux, "return core::system::pselect", "select outer monitor path directly returns pselect");
    ok &= requireContains(epollMux, "return core::system::epoll_pwait", "epoll outer monitor path directly returns epoll_pwait");
    ok &= requireContains(coreMux, "activeDescriptorCount = monitorDescriptors", "generic wait path consumes monitorDescriptors directly");
    ok &= requireContains(coreMux, "if (activeDescriptorCount < 0)", "generic wait path checks negative syscall result immediately");
    ok &= requireContains(epollWrapper, "return ::epoll_pwait", "epoll_pwait wrapper remains direct pass-through");
    ok &= requireContains(pollWrapper, "return ::ppoll", "ppoll wrapper remains direct pass-through");
    ok &= requireContains(selectWrapper, "return ::pselect", "pselect wrapper remains direct pass-through");

    return ok ? 0 : 1;
}
