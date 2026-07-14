#include "log/Logger.h"
#include "utils/hexdump.h"
#include "tests/support/TestResult.h"

#include <string>
#include <vector>

namespace {
    std::string stripAllowedSgr(std::string value) {
        const std::string allowed[] = {"\033[32m", "\033[33m", "\033[34m", "\033[39m", "\033[0m"};
        for (const auto& sgr : allowed) {
            std::size_t pos = 0;
            while ((pos = value.find(sgr, pos)) != std::string::npos) {
                value.erase(pos, sgr.size());
            }
        }
        return value;
    }
}

int main() {
    tests::support::TestResult result;
    const std::vector<char> bytes{'A', '\0', '\x1b', 'Z', '\n', static_cast<char>(0xff)};

    const std::string plain = utils::hexDump(bytes, 4, true);
    const auto presentation = utils::hexDumpPresentation(bytes, 4, true);
    result.expectTrue(plain == presentation.plain, "hexDump remains the deterministic plain presentation");
    result.expectTrue(plain.find('\033') == std::string::npos, "plain hex dump contains no ESC bytes");
    result.expectTrue(plain.find("\\x1B[") == std::string::npos, "plain hex dump contains no literal color clutter");
    result.expectTrue(presentation.terminal.find("\033[34m") != std::string::npos, "terminal hex dump colors offsets");
    result.expectTrue(presentation.terminal.find("\033[32m") != std::string::npos, "terminal hex dump colors bytes");
    result.expectTrue(presentation.terminal.find("\033[33m") != std::string::npos, "terminal hex dump colors ASCII");
    result.expectTrue(stripAllowedSgr(presentation.terminal) == plain, "stripping allowed SGR reproduces plain dump exactly");

    logger::Logger::setDisableColor(true);
    const auto disabled = utils::hexDumpPresentation(bytes, 4, true);
    logger::Logger::setDisableColor(false);
    const auto enabled = utils::hexDumpPresentation(bytes, 4, true);
    result.expectTrue(disabled.plain == enabled.plain && disabled.terminal == enabled.terminal,
                      "hex dump presentation is independent of Logger color state");

    const auto empty = utils::hexDumpPresentation(std::vector<char>{});
    result.expectTrue(empty.plain.empty() && empty.terminal.empty(), "empty input produces empty presentations");

    const std::vector<char> partial{'0', '1', '2'};
    const auto partialDump = utils::hexDumpPresentation(partial, 2, true);
    result.expectTrue(stripAllowedSgr(partialDump.terminal) == partialDump.plain, "partial rows differ only by SGR tokens");

    return result.processResult();
}
