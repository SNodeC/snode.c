#include "SourcePolicyTestRoot.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

    bool requireContains(std::string_view source, std::string_view fragment, std::string_view description) {
        if (source_policy::contains(source, fragment)) {
            return true;
        }

        std::cerr << "Missing inner epoll diagnostic evidence: " << description << "\nExpected fragment: " << fragment << '\n';
        return false;
    }

    bool requireAbsent(std::string_view source, std::string_view fragment, std::string_view description) {
        if (!source_policy::contains(source, fragment)) {
            return true;
        }

        std::cerr << "Forbidden inner epoll diagnostic pattern: " << description << "\nForbidden fragment: " << fragment << '\n';
        return false;
    }

} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }
    const std::filesystem::path sourcePath = root / "src" / "core" / "multiplexer" / "epoll" / "DescriptorEventPublisher.cpp";
    const std::string source = source_policy::readSourcePolicyFile(sourcePath);
    if (source.empty()) {
        std::cerr << "Unable to read " << sourcePath << '\n';
        return 1;
    }

    bool ok = true;
    ok &= requireContains(source, "epfd = core::system::epoll_create1(EPOLL_CLOEXEC);", "inner epoll_create1 call remains local");
    ok &= requireContains(source, "if (epfd < 0)", "invalid epfd guard after create and before follow-on syscalls");
    ok &= requireContains(source, "core.mux epoll_create1 failed", "epoll_create1 failure is logged");
    ok &= requireContains(source, "const int errnum = errno;", "errno is captured immediately for failing syscalls");
    ok &= requireContains(source, "EPOLL_CTL_ADD", "ADD operation remains explicit");
    ok &= requireContains(source, "errnum == EEXIST", "EEXIST ADD fallback is preserved");
    ok &= requireContains(source, "core.mux epoll_ctl ADD failed: fd={}", "ADD failures other than EEXIST are logged");
    ok &= requireContains(source, "EPOLL_CTL_DEL", "DEL operation remains explicit");
    ok &= requireContains(source, "errnum == EBADF", "EBADF cleanup tolerance is preserved");
    ok &= requireContains(source, "core.mux epoll_ctl DEL failed: fd={}", "DEL failures other than EBADF are logged");
    ok &= requireContains(source, "EPOLL_CTL_MOD", "MOD operation remains explicit");
    ok &= requireContains(source, "core.mux epoll_ctl MOD failed: fd={}", "MOD failures are logged");
    ok &= requireContains(source, "const int count = core::system::epoll_wait", "inner non-blocking epoll_wait remains local");
    ok &= requireContains(source, "if (count < 0)", "epoll_wait failure count is checked");
    ok &= requireContains(source, "errnum != EINTR", "EINTR wait failures are not escalated as system errors");
    ok &= requireContains(source, "core.mux epoll_wait failed: interestCount={}", "non-EINTR wait failures are logged");
    ok &= requireContains(source, ".sysError(logger::LogLevel::Error, errnum", "errno-backed sysError diagnostics are used");
    ok &= requireContains(source, "if (interestCount > 0)", "unsigned interestCount decrement is guarded");

    ok &= requireAbsent(source,
                        "core.mux epoll_ctl DEL failed: fd={}\", fd);\n                interestCount--",
                        "real DEL failure path must not decrement unsigned interestCount");
    ok &= requireAbsent(source, "--interestCount", "raw pre-decrement can underflow unsigned interestCount");
    ok &= requireAbsent(source, "interestCount -=", "compound decrement can underflow unsigned interestCount");

    return ok ? 0 : 1;
}
