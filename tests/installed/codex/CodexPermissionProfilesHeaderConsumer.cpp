// Compile the installed public header twice in one otherwise isolated
// translation unit to prove conventional include-guard behavior.
// clang-format off
#include <ai/openai/codex/typed/PermissionProfiles.h>
#include <ai/openai/codex/typed/PermissionProfiles.h>
// clang-format on

#include <cstdint>
#include <string>
#include <type_traits>

int main() {
    namespace typed = ai::openai::codex::typed;

    using List = typed::PermissionProfiles::Submission (typed::PermissionProfiles::*)(typed::PermissionProfileListParams,
                                                                                      typed::PermissionProfiles::ListResultHandler);
    static_assert(std::is_same_v<decltype(&typed::PermissionProfiles::list), List>);

    const typed::PermissionProfileListParams params{
        .cursor = typed::OptionalNullable<std::string>::explicitNull(),
        .cwd = typed::OptionalNullable<std::string>::withValue("/synthetic/project"),
        .limit = typed::OptionalNullable<std::uint32_t>::withValue(25),
    };
    const typed::PermissionProfileSummary profile{
        .allowed = true,
        .description = typed::OptionalNullable<std::string>::withValue("Synthetic profile"),
        .id = "synthetic-profile",
    };
    return params.cursor.isNull() && profile.allowed ? 0 : 1;
}
