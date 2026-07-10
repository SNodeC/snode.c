/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "Cli.h"

#include "ArgTokenizer.h"
#include "CommandBuilder.h"
#include "ConfigActions.h"
#include "ConfigEditor.h"
#include "ConfigModel.h"
#include "ConfigParser.h"
#include "JsonWriter.h"
#include "Materializer.h"
#include "ProcessRunner.h"
#include "ui/Ui.h"

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace snodec::control {

    namespace {

        struct Options {
            std::optional<std::string> target;
            std::string targetArgs;
            bool dumpModel = false;
            bool printSummary = false;
            std::optional<std::string> writeTemplatePath;
            std::optional<std::string> materializePath; // may be "-" for stdout
            bool listOptions = false;
            std::optional<std::string> getKey;
            std::vector<EditOperation> editOps;
            bool allowNewOptions = false;
            bool checkRequired = false;
            bool diff = false;
            std::optional<std::string> saveConfigPath;
            std::optional<std::string> runConfigPath;
            bool run = false;
            bool printRunCommand = false;
            bool dryRun = false;
            bool keepTemp = false;
            bool ui = false;
            bool help = false;
            std::string usageError;
        };

        const char* usageText =
            "snodec-control - Out-of-tree inspection & configuration workflow tool for SNode.C applications\n"
            "\n"
            "Usage:\n"
            "  snodec-control --target <path> [options]\n"
            "\n"
            "Discovery:\n"
            "  --target <path>             Path to the target SNode.C application executable (required)\n"
            "  --target-args \"<args>\"       Extra arguments passed to the target before '-s'/'-w'/run\n"
            "\n"
            "Inspection:\n"
            "  --dump-model                 Print the parsed configuration model as JSON\n"
            "  --print-summary              Print a human-readable summary of the configuration\n"
            "  --list-options               Print a stable, readable list of every parsed option\n"
            "  --get <key>                  Print one option, looked up by full key or bare key\n"
            "  --write-template <file>      Write the target's raw '-s' output to <file>\n"
            "\n"
            "Editing:\n"
            "  --set <key=value>            Set an option's active value (repeatable)\n"
            "  --unset <key>                Clear an option's active value, falling back to default (repeatable)\n"
            "  --allow-new-options          Allow --set/--unset to create unknown keys instead of erroring\n"
            "  --diff                       Print a summary of the changes made by --set/--unset (and --ui, if used)\n"
            "  --check-required             Fail if any required option still has no usable value\n"
            "  --materialize <file>         Write a clean, editable config file derived from the (edited) model\n"
            "                               Use '-' to write to stdout instead of a file\n"
            "\n"
            "Interactive UI:\n"
            "  --ui                         Open a hierarchical, curses-based interactive configuration editor\n"
            "                               after applying any --set/--unset given on the command line. Further\n"
            "                               --save-config/--run/--diff/--check-required/--materialize requested on\n"
            "                               the same command line are applied afterwards, using the edited model.\n"
            "                               Fails clearly if this build has no Curses support.\n"
            "\n"
            "Saving & running:\n"
            "  --save-config <file>         Ask the target to write its canonical config file, from the (edited) model\n"
            "  --run-config <file>          Use an existing config file for --run / --print-run-command\n"
            "  --run                        Run the target (config file resolution: --run-config, else the file just\n"
            "                               written by --save-config, else a temporary materialized config if edits\n"
            "                               were given, else no config file at all)\n"
            "  --print-run-command          Print the exact command --run would execute, without running it\n"
            "  --keep-temp                  Keep and print the path of any temporary materialized config file\n"
            "  --dry-run                    Discover, edit, and check, but do not save, run, or write output files\n"
            "                               (except stdout output explicitly requested with --materialize -)\n"
            "\n"
            "Other:\n"
            "  -h, --help                   Show this help text and exit\n"
            "\n"
            "Examples:\n"
            "  snodec-control --target /path/to/mqttbroker --print-summary\n"
            "  snodec-control --target /path/to/mqttbroker --list-options\n"
            "  snodec-control --target /path/to/mqttbroker --get echoserver.local.port\n"
            "  snodec-control --target /path/to/mqttbroker --set echoserver.local.port=8080 --set log-level=5 --diff\n"
            "  snodec-control --target /path/to/mqttbroker --set echoserver.local.port=8080 --materialize /tmp/edited.conf\n"
            "  snodec-control --target /path/to/mqttbroker --set echoserver.local.port=8080 --save-config /tmp/app.conf\n"
            "  snodec-control --target /path/to/mqttbroker --run-config /tmp/app.conf --run\n"
            "  snodec-control --target /path/to/mqttbroker --set echoserver.local.port=8080 --save-config /tmp/app.conf --run\n"
            "  snodec-control --target /path/to/mqttbroker --run-config /tmp/app.conf --print-run-command\n"
            "  snodec-control --target /path/to/mqttbroker --ui\n"
            "  snodec-control --target /path/to/mqttbroker --ui --save-config /tmp/app.conf --run\n"
            "\n"
            "snodec-control never dlopen()s the target and never calls its main(): it always inspects, saves through,\n"
            "and runs the target by executing it out-of-process, using '-s'/'--show-config' for discovery and the\n"
            "target's own '--config-file'/'--write-config' options for saving. snodec-control itself is a standalone,\n"
            "out-of-tree project: it is never built as part of the SNode.C source tree and links against none of its\n"
            "libraries.\n";

        bool takesValue(const std::string& option) {
            return option == "--target" || option == "--target-args" || option == "--write-template" || option == "--materialize" ||
                   option == "--get" || option == "--set" || option == "--unset" || option == "--save-config" || option == "--run-config";
        }

        Options parseArgs(int argc, char* argv[]) {
            Options options;
            std::vector<std::string> args(argv + 1, argv + argc);

            for (std::size_t i = 0; i < args.size(); ++i) {
                std::string arg = args[i];
                std::string value;
                bool haveInlineValue = false;

                // Only "--flag=value" style is split here for flags whose value cannot itself contain an
                // '=': --set/--unset intentionally always take a separate argv token, since --set's own
                // value is itself a "key=value" pair (e.g. --set=foo=bar would be ambiguous to split).
                const std::size_t equalsPos = arg.find('=');
                if (arg.rfind("--", 0) == 0 && equalsPos != std::string::npos) {
                    const std::string candidateFlag = arg.substr(0, equalsPos);
                    if (candidateFlag == "--target" || candidateFlag == "--target-args" || candidateFlag == "--write-template" ||
                        candidateFlag == "--materialize" || candidateFlag == "--get" || candidateFlag == "--save-config" ||
                        candidateFlag == "--run-config") {
                        value = arg.substr(equalsPos + 1);
                        arg = candidateFlag;
                        haveInlineValue = true;
                    }
                }

                if (arg == "-h" || arg == "--help") {
                    options.help = true;
                    continue;
                }

                if (takesValue(arg)) {
                    if (!haveInlineValue) {
                        if (i + 1 >= args.size()) {
                            options.usageError = "Missing value for option '" + arg + "'";
                            return options;
                        }
                        value = args[++i];
                    }

                    if (arg == "--target") {
                        options.target = value;
                    } else if (arg == "--target-args") {
                        options.targetArgs = value;
                    } else if (arg == "--write-template") {
                        options.writeTemplatePath = value;
                    } else if (arg == "--materialize") {
                        options.materializePath = value;
                    } else if (arg == "--get") {
                        options.getKey = value;
                    } else if (arg == "--save-config") {
                        options.saveConfigPath = value;
                    } else if (arg == "--run-config") {
                        options.runConfigPath = value;
                    } else if (arg == "--set") {
                        std::string key;
                        std::string setValue;
                        if (!parseSetArgument(value, key, setValue)) {
                            options.usageError = "Malformed --set argument (expected key=value): '" + value + "'";
                            return options;
                        }
                        options.editOps.push_back(EditOperation{EditOperation::Kind::Set, key, setValue});
                    } else if (arg == "--unset") {
                        options.editOps.push_back(EditOperation{EditOperation::Kind::Unset, value, ""});
                    }
                    continue;
                }

                if (arg == "--dump-model") {
                    options.dumpModel = true;
                } else if (arg == "--print-summary") {
                    options.printSummary = true;
                } else if (arg == "--list-options") {
                    options.listOptions = true;
                } else if (arg == "--allow-new-options") {
                    options.allowNewOptions = true;
                } else if (arg == "--check-required") {
                    options.checkRequired = true;
                } else if (arg == "--diff") {
                    options.diff = true;
                } else if (arg == "--run") {
                    options.run = true;
                } else if (arg == "--print-run-command") {
                    options.printRunCommand = true;
                } else if (arg == "--dry-run") {
                    options.dryRun = true;
                } else if (arg == "--keep-temp") {
                    options.keepTemp = true;
                } else if (arg == "--ui") {
                    options.ui = true;
                } else {
                    options.usageError = "Unknown option '" + arg + "'";
                    return options;
                }
            }

            return options;
        }

        std::string joinCandidates(const std::vector<std::string>& candidates) {
            std::string joined;
            for (std::size_t i = 0; i < candidates.size(); ++i) {
                joined += (i == 0 ? "" : ", ") + candidates[i];
            }
            return joined;
        }

    } // namespace

    int runCli(int argc, char* argv[]) {
        const Options options = parseArgs(argc, argv);

        if (options.help || argc <= 1) {
            std::cout << usageText;
            return EXIT_CODE_OK;
        }

        if (!options.usageError.empty()) {
            std::cerr << "Error: " << options.usageError << "\n\n" << usageText;
            return EXIT_CODE_USAGE_ERROR;
        }

        if (!options.target.has_value()) {
            std::cerr << "Error: missing required --target <path>\n\n" << usageText;
            return EXIT_CODE_USAGE_ERROR;
        }

        const bool haveAction = options.dumpModel || options.printSummary || options.writeTemplatePath.has_value() ||
                                options.materializePath.has_value() || options.listOptions || options.getKey.has_value() ||
                                options.checkRequired || options.diff || options.saveConfigPath.has_value() || options.run ||
                                options.printRunCommand || options.ui;

        if (!haveAction) {
            std::cerr << "Error: no action requested. Specify at least one of --dump-model, --print-summary, "
                         "--list-options, --get <key>, --write-template <file>, --materialize <file>, --diff, "
                         "--check-required, --save-config <file>, --run, --print-run-command, --ui.\n\n"
                      << usageText;
            return EXIT_CODE_USAGE_ERROR;
        }

        if (options.dryRun && options.run) {
            std::cerr << "Note: --dry-run is set; --run will not actually execute the target.\n";
        }

        const std::string targetPath = *options.target;
        std::error_code fsError;
        const bool exists = std::filesystem::exists(targetPath, fsError);
        if (!exists) {
            std::cerr << "Error: target path does not exist: " << targetPath << "\n";
            return EXIT_CODE_TARGET_ERROR;
        }

        if (::access(targetPath.c_str(), X_OK) != 0) {
            std::cerr << "Error: target is not executable: " << targetPath << "\n";
            return EXIT_CODE_TARGET_ERROR;
        }

        const std::vector<std::string> targetArgTokens = tokenizeArgs(options.targetArgs);

        const DiscoveryOutcome discovery = discoverTarget(targetPath, targetArgTokens);
        if (!discovery.ok) {
            std::cerr << discovery.fatalError;
            return EXIT_CODE_EXECUTION_ERROR;
        }
        std::cerr << discovery.diagnostics;

        ParseResult parseResult = discovery.parseResult;

        const EditOutcome editOutcome = applyEdits(parseResult.model, options.editOps, options.allowNewOptions);
        if (!editOutcome.errors.empty()) {
            for (const EditError& error : editOutcome.errors) {
                std::cerr << "Error: " << error.message << "\n";
            }
            return EXIT_CODE_EDIT_ERROR;
        }

        int exitCode = EXIT_CODE_OK;
        std::vector<ChangeRecord> effectiveChanges = editOutcome.changes;
        std::optional<std::string> savedConfigPathForRun;

        if (options.ui) {
            if (!ui::isUiAvailable()) {
                std::cerr << "Error: Interactive UI support was not built because Curses was not found.\n";
                exitCode = EXIT_CODE_UI_ERROR;
            } else {
                ui::UiOptions uiOptions;
                uiOptions.targetPath = targetPath;
                uiOptions.targetArgTokens = targetArgTokens;
                uiOptions.initialSaveConfigPath = options.saveConfigPath;
                uiOptions.dryRun = options.dryRun;
                uiOptions.keepTemp = options.keepTemp;
                uiOptions.priorChanges = editOutcome.changes;
                uiOptions.metadata = discovery.metadata;

                const ui::UiResult uiResult = ui::runInteractiveUi(parseResult.model, uiOptions);
                if (!uiResult.message.empty()) {
                    std::cerr << uiResult.message;
                }
                if (!uiResult.ok) {
                    exitCode = EXIT_CODE_UI_ERROR;
                } else {
                    effectiveChanges = mergeChangeRecords(editOutcome.changes, uiResult.changes);
                    savedConfigPathForRun = uiResult.savedConfigPathForRun;
                }
            }
        }

        if (options.dumpModel) {
            std::cout << toJson(parseResult.model, targetPath);
        }

        if (options.printSummary) {
            std::cout << formatSummary(parseResult.model, targetPath);
        }

        if (options.listOptions) {
            std::cout << formatListOptions(parseResult.model);
        }

        if (options.getKey.has_value()) {
            const LookupResult lookup = findOption(parseResult.model, *options.getKey);
            if (lookup.status == LookupStatus::Found) {
                std::cout << formatOptionBlock(*lookup.option);
            } else if (lookup.status == LookupStatus::Ambiguous) {
                std::cerr << "Error: ambiguous option '" << *options.getKey << "', candidates: " << joinCandidates(lookup.candidates)
                          << "\n";
                exitCode = EXIT_CODE_EDIT_ERROR;
            } else {
                std::cerr << "Error: unknown option '" << *options.getKey << "'\n";
                exitCode = EXIT_CODE_EDIT_ERROR;
            }
        }

        if (options.diff) {
            std::cout << formatDiff(effectiveChanges);
        }

        if (options.checkRequired) {
            const std::vector<const ConfigOption*> missing = missingRequiredOptions(parseResult.model);
            if (missing.empty()) {
                std::cout << "All required options have a usable value.\n";
            } else {
                std::cerr << formatMissingRequired(missing);
                exitCode = EXIT_CODE_REQUIRED_MISSING;
            }
        }

        if (options.writeTemplatePath.has_value()) {
            if (options.dryRun) {
                std::cerr << "[dry-run] Would write raw configuration template to " << *options.writeTemplatePath << "\n";
            } else {
                std::string error;
                if (!writeFile(*options.writeTemplatePath, discovery.rawStdOut, error)) {
                    std::cerr << "Error: " << error << "\n";
                    exitCode = EXIT_CODE_OUTPUT_ERROR;
                } else {
                    std::cerr << "Wrote raw configuration template to " << *options.writeTemplatePath << "\n";
                }
            }
        }

        if (options.materializePath.has_value()) {
            const std::string materialized = materialize(parseResult.model, targetPath);
            if (*options.materializePath == "-") {
                std::cout << materialized;
            } else if (options.dryRun) {
                std::cerr << "[dry-run] Would write materialized configuration to " << *options.materializePath << "\n";
            } else {
                std::string error;
                if (!writeFile(*options.materializePath, materialized, error)) {
                    std::cerr << "Error: " << error << "\n";
                    exitCode = EXIT_CODE_OUTPUT_ERROR;
                } else {
                    std::cerr << "Wrote materialized configuration to " << *options.materializePath << "\n";
                }
            }
        }

        if (options.saveConfigPath.has_value()) {
            const SaveOutcome saveOutcome = performSaveConfig(
                parseResult.model, targetPath, targetArgTokens, *options.saveConfigPath, options.dryRun, options.keepTemp);
            if (saveOutcome.isError) {
                std::cerr << saveOutcome.message;
                exitCode = EXIT_CODE_SAVE_ERROR;
            } else {
                std::cerr << saveOutcome.message;
            }
            if (saveOutcome.succeeded) {
                savedConfigPathForRun = options.saveConfigPath;
            }
        }

        if (options.run || options.printRunCommand) {
            const bool haveEdits = !options.editOps.empty() || !effectiveChanges.empty();
            const RunConfigResolution resolution = resolveRunConfigPath(
                options.runConfigPath, options.saveConfigPath, savedConfigPathForRun, haveEdits, options.dryRun, parseResult.model,
                targetPath);

            if (!resolution.ok) {
                std::cerr << "Error: " << resolution.error << "\n";
                exitCode = EXIT_CODE_RUN_ERROR;
            } else {
                const auto runArgsOpt = buildRunArgs(targetArgTokens, resolution.configPath);
                if (!runArgsOpt) {
                    std::cerr << "Error: --target-args already specifies a config file; refusing to also append "
                                 "'--config-file "
                              << *resolution.configPath << "' (conflict).\n";
                    exitCode = EXIT_CODE_RUN_ERROR;
                } else {
                    if (options.printRunCommand) {
                        std::cout << formatCommandForDisplay(targetPath, *runArgsOpt) << "\n";
                    }

                    if (options.run) {
                        if (options.dryRun) {
                            std::cerr << "[dry-run] Would run: " << formatCommandForDisplay(targetPath, *runArgsOpt) << "\n";
                        } else {
                            const ProcessResult runResult = runProcessAttached(targetPath, *runArgsOpt);
                            if (!runResult.spawned) {
                                std::cerr << "Error: failed to execute target '" << targetPath << "': " << runResult.spawnError << "\n";
                                exitCode = EXIT_CODE_RUN_ERROR;
                            } else if (runResult.signaled) {
                                std::cerr << "Target was terminated by signal " << runResult.signalNumber << ".\n";
                                exitCode = 128 + runResult.signalNumber;
                            } else {
                                exitCode = runResult.exitCode;
                            }
                        }
                    }
                }
            }

            if (!resolution.tempPathCreated.empty()) {
                if (options.keepTemp) {
                    std::cerr << "Kept temporary run config at " << resolution.tempPathCreated << "\n";
                } else {
                    std::filesystem::remove(resolution.tempPathCreated);
                }
            }
        }

        return exitCode;
    }

} // namespace snodec::control
