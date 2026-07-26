// Compile the installed public header twice in one otherwise isolated
// translation unit to prove conventional include-guard behavior.
// clang-format off
#include <ai/openai/codex/typed/Configuration.h>
#include <ai/openai/codex/typed/Configuration.h>
// clang-format on

#include <type_traits>

int main() {
    namespace codex = ai::openai::codex;
    namespace typed = ai::openai::codex::typed;

    using BatchWrite = typed::Configuration::Submission (typed::Configuration::*)(typed::ConfigBatchWriteParams,
                                                                                  typed::Configuration::BatchWriteResultHandler);
    using ReloadMcp = typed::Configuration::Submission (typed::Configuration::*)(typed::Unit, typed::Configuration::UnitResultHandler);
    using Read =
        typed::Configuration::Submission (typed::Configuration::*)(typed::ConfigReadParams, typed::Configuration::ReadResultHandler);
    using ReadRequirements =
        typed::Configuration::Submission (typed::Configuration::*)(typed::Unit, typed::Configuration::ReadRequirementsResultHandler);
    using WriteValue = typed::Configuration::Submission (typed::Configuration::*)(typed::ConfigValueWriteParams,
                                                                                  typed::Configuration::WriteValueResultHandler);
    using SetFeature = typed::Configuration::Submission (typed::Configuration::*)(
        typed::ExperimentalFeatureEnablementSetParams, typed::Configuration::SetExperimentalFeatureEnablementResultHandler);
    using ListFeatures = typed::Configuration::Submission (typed::Configuration::*)(
        typed::ExperimentalFeatureListParams, typed::Configuration::ListExperimentalFeaturesResultHandler);

    static_assert(std::is_same_v<decltype(&typed::Configuration::batchWrite), BatchWrite>);
    static_assert(std::is_same_v<decltype(&typed::Configuration::reloadMcpServers), ReloadMcp>);
    static_assert(std::is_same_v<decltype(&typed::Configuration::read), Read>);
    static_assert(std::is_same_v<decltype(&typed::Configuration::readRequirements), ReadRequirements>);
    static_assert(std::is_same_v<decltype(&typed::Configuration::writeValue), WriteValue>);
    static_assert(std::is_same_v<decltype(&typed::Configuration::setExperimentalFeatureEnablement), SetFeature>);
    static_assert(std::is_same_v<decltype(&typed::Configuration::listExperimentalFeatures), ListFeatures>);

    typed::ConfigValueWriteParams value;
    value.keyPath = typed::ConfigKeyPath{"installed.header.check"};
    value.mergeStrategy = typed::MergeStrategy::replace();
    value.value = codex::Json::array({true, nullptr, codex::Json{{"exact", 1}}});
    return value.expectedVersion.isOmitted() && value.value.has_value() ? 0 : 1;
}
