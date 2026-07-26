// Compile the installed public header twice in one otherwise isolated
// translation unit to prove conventional include-guard behavior.
// clang-format off
#include <ai/openai/codex/typed/Models.h>
#include <ai/openai/codex/typed/Models.h>
// clang-format on

#include <type_traits>

int main() {
    namespace typed = ai::openai::codex::typed;

    using List = typed::Models::Submission (typed::Models::*)(typed::ModelListParams, typed::Models::ListResultHandler);
    using ReadProviderCapabilities = typed::Models::Submission (typed::Models::*)(typed::ModelProviderCapabilitiesReadParams,
                                                                                  typed::Models::ReadProviderCapabilitiesResultHandler);

    static_assert(std::is_same_v<decltype(&typed::Models::list), List>);
    static_assert(std::is_same_v<decltype(&typed::Models::readProviderCapabilities), ReadProviderCapabilities>);

    const typed::ModelRerouteReason futureReason{"installed-future-reroute"};
    return futureReason.isKnown() ? 1 : 0;
}
