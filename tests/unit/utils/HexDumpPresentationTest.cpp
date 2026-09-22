#include "log/Logger.h"
#include "tests/support/TestResult.h"
#include "utils/hexdump.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
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

    result.expectTrue(utils::hexDump(std::string("Hello, MQTT!\r\n\0\xff", 16)) ==
                          "00000000  48 65 6c 6c 6f 2c 20 4d  51 54 54 21 0d 0a 00 ff  |Hello, MQTT!....|",
                      "sixteen-byte rows have grouped hex and aligned ASCII");
    for (const std::size_t length : {0u, 1u, 7u, 8u, 15u, 16u, 17u, 31u, 32u, 256u, 4097u}) {
        std::string input(length, '\0');
        for (std::size_t i = 0; i < length; ++i)
            input[i] = static_cast<char>(i % 256);
        const auto dump = utils::hexDumpPresentation(input);
        result.expectTrue(stripAllowedSgr(dump.terminal) == dump.plain, "all row boundaries preserve plain/color equivalence");
        std::istringstream rows(dump.plain);
        std::size_t offset = 0;
        for (std::string row; std::getline(rows, row);) {
            result.expectEqual(std::size_t{78}, row.size(), "every row occupies 78 columns before the log continuation marker");
            if (row.size() != 78)
                continue;
            std::ostringstream address;
            address << std::hex << std::setfill('0') << std::setw(8) << offset;
            result.expectTrue(row.substr(0, 8) == address.str(), "offset identifies the source row");
            const auto count = std::min<std::size_t>(16, length - offset);
            for (std::size_t column = 0; column < 16; ++column) {
                const auto position = 10 + column * 3 + (column >= 8 ? 1 : 0);
                if (column < count) {
                    const auto byte = static_cast<unsigned char>(input[offset + column]);
                    std::ostringstream hex;
                    hex << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(byte);
                    result.expectTrue(row.substr(position, 2) == hex.str(), "each input byte appears exactly in its hex column");
                    result.expectTrue(row[61 + column] == (byte >= 32 && byte <= 126 ? static_cast<char>(byte) : '.'),
                                      "ASCII includes only printable bytes");
                } else {
                    result.expectTrue(row.substr(position, 2) == "  " && row[61 + column] == ' ', "partial row padding is aligned");
                }
            }
            offset += count;
        }
        result.expectEqual(length, offset, "no payload bytes are truncated");
        const auto colorSwitches = static_cast<std::size_t>(std::count(dump.terminal.begin(), dump.terminal.end(), '\033'));
        result.expectEqual(((length + 15) / 16) * 6, colorSwitches, "color switches are per section, not per byte");
    }

    return result.processResult();
}
