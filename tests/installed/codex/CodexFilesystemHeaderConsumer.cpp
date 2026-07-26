// Compile the installed public header twice in one otherwise isolated
// translation unit to prove conventional include-guard behavior.
// clang-format off
#include <ai/openai/codex/typed/Filesystem.h>
#include <ai/openai/codex/typed/Filesystem.h>
// clang-format on

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

int main() {
    namespace typed = ai::openai::codex::typed;

    using Copy = typed::Filesystem::Submission (typed::Filesystem::*)(typed::FsCopyParams, typed::Filesystem::UnitResultHandler);
    using Search =
        typed::Filesystem::Submission (typed::Filesystem::*)(typed::FuzzyFileSearchParams, typed::Filesystem::FuzzyFileSearchResultHandler);

    static_assert(std::is_same_v<decltype(&typed::Filesystem::copy), Copy>);
    static_assert(std::is_same_v<decltype(&typed::Filesystem::fuzzyFileSearch), Search>);
    static_assert(std::is_same_v<decltype(typed::FuzzyFileSearchResult::score), std::uint32_t>);

    const typed::FsCopyParams copy{
        .destinationPath = {"/synthetic/installed-destination"},
        .recursive = true,
        .sourcePath = {"/synthetic/installed-source"},
    };
    const typed::FuzzyFileSearchParams search{
        .cancellationToken = typed::OptionalNullable<std::string>::withValue("synthetic-cancellation"),
        .query = "synthetic-query",
        .roots = {"/synthetic/installed-root"},
    };
    const typed::FsChangedNotification changed{
        .changedPaths = {{"/synthetic/installed-root/file"}},
        .watchId = {"synthetic-watch"},
    };
    [[maybe_unused]] const typed::OperationResult<typed::FuzzyFileSearchResponse> result;

    return copy.recursive.value_or(false) && search.cancellationToken.hasValue() && changed.changedPaths.size() == 1 ? 0 : 1;
}
