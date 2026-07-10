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

#include "ConfigActions.h"

#include "CommandBuilder.h"
#include "Materializer.h"
#include "ProcessRunner.h"

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <unistd.h>

namespace snodec::control {

    namespace {

        std::string firstLineOf(const std::string& text) {
            const std::size_t pos = text.find('\n');
            return pos == std::string::npos ? text : text.substr(0, pos);
        }

        // Renders an optional string field for --list-options/--get: "(none)" when absent, and a
        // visibly-quoted "" when present but empty, so the two states are never confused.
        std::string displayField(bool present, const std::string& value) {
            if (!present) {
                return "(none)";
            }
            return value.empty() ? "\"\"" : value;
        }

    } // namespace

    DiscoveryOutcome discoverTarget(const std::string& targetPath, const std::vector<std::string>& targetArgTokens) {
        DiscoveryOutcome outcome;

        const ProcessResult discoveryResult = runProcess(targetPath, buildDiscoveryArgs(targetArgTokens));

        if (!discoveryResult.spawned) {
            outcome.fatalError = "Error: failed to execute target '" + targetPath + "': " + discoveryResult.spawnError + "\n";
            return outcome;
        }

        if (discoveryResult.stdOut.empty()) {
            std::ostringstream error;
            error << "Error: target '" << targetPath << "' produced no output on stdout for '-s'.\n";
            if (!discoveryResult.stdErr.empty()) {
                error << "Target stderr:\n" << discoveryResult.stdErr;
            }
            if (discoveryResult.signaled) {
                error << "Target was terminated by signal " << discoveryResult.signalNumber << ".\n";
            } else {
                error << "Target exit code: " << discoveryResult.exitCode << "\n";
            }
            outcome.fatalError = error.str();
            return outcome;
        }

        std::ostringstream diagnostics;
        if (discoveryResult.signaled) {
            diagnostics << "Warning: target was terminated by signal " << discoveryResult.signalNumber
                        << " but produced usable '-s' output; proceeding.\n";
        } else if (discoveryResult.exitCode != 0) {
            diagnostics << "Note: target exited with status " << discoveryResult.exitCode
                        << " while handling '-s' (this is expected for SNode.C applications, which never proceed to "
                           "run when '-s' is given); usable configuration output was captured and will be parsed.\n";
        }

        outcome.rawStdOut = discoveryResult.stdOut;
        outcome.parseResult = parseShowConfigOutput(discoveryResult.stdOut);
        for (const std::string& warning : outcome.parseResult.warnings) {
            diagnostics << "Warning: " << warning << "\n";
        }

        outcome.metadata = parseMetaBlocks(discoveryResult.stdOut);
        if (outcome.metadata.present && !outcome.metadata.schemaRecognized) {
            diagnostics << "Note: target emitted '#@' configuration metadata with an unrecognized schema/version; "
                           "falling back to plain '-s' parsing for hierarchy and type information.\n";
        }
        for (const std::string& warning : outcome.metadata.warnings) {
            diagnostics << "Warning (metadata): " << warning << "\n";
        }
        applyMetadataToModel(outcome.parseResult.model, outcome.metadata);

        outcome.diagnostics = diagnostics.str();
        outcome.ok = true;
        return outcome;
    }

    std::string formatSummary(const ConfigModel& model, const std::string& targetPath) {
        std::ostringstream out;
        out << "snodec-control summary\n";
        out << "=======================\n";
        out << "Target:             " << targetPath << "\n";
        out << "Sections:           " << model.sectionCount() << "\n";
        out << "Options (total):    " << model.optionCount() << "\n";
        out << "Required options:   " << model.requiredOptionCount() << "\n";
        out << "Active options:     " << model.activeOptionCount() << "\n";
        out << "\n";
        out << "Sections:\n";
        for (const ConfigSection& section : model.sections) {
            out << "  - " << (section.name.empty() ? "(global)" : section.name) << ": " << section.options.size() << " option(s)\n";
        }
        return out.str();
    }

    std::string formatOptionBlock(const ConfigOption& option) {
        std::ostringstream out;
        out << fullOptionKey(option) << "\n";
        out << "  section:     " << (option.section.empty() ? "(global)" : option.section) << "\n";
        out << "  active:      " << displayField(option.hasActiveValue, option.activeValue.value_or("")) << "\n";
        out << "  default:     " << displayField(option.defaultValue.has_value(), option.defaultValue.value_or("")) << "\n";
        out << "  required:    " << (option.required ? "yes" : "no");
        if (option.requiredSource.has_value()) {
            out << " (" << *option.requiredSource << "; current target state only, not a canonical/suppression-aware value)";
        }
        out << "\n";
        if (option.typeKind.has_value()) {
            out << "  type:        " << *option.typeKind;
            if (option.typeItems.has_value() && *option.typeItems == "list") {
                out << ", list";
            }
            if (option.typeKindSource.has_value() && option.typeKindSource->rfind("heuristic", 0) == 0) {
                out << " (heuristic: " << *option.typeKindSource << ", not certain)";
            }
            out << "\n";
        }
        if (!option.needs.empty()) {
            out << "  needs:       ";
            for (std::size_t i = 0; i < option.needs.size(); ++i) {
                out << (i > 0 ? ", " : "") << option.needs[i];
            }
            out << "\n";
        }
        if (!option.excludes.empty()) {
            out << "  excludes:    ";
            for (std::size_t i = 0; i < option.excludes.size(); ++i) {
                out << (i > 0 ? ", " : "") << option.excludes[i];
            }
            out << "\n";
        }
        if (!option.description.empty()) {
            out << "  description: " << firstLineOf(option.description) << "\n";
        }
        return out.str();
    }

    std::string formatListOptions(const ConfigModel& model) {
        std::ostringstream out;
        bool first = true;
        for (const ConfigSection& section : model.sections) {
            for (const ConfigOption& option : section.options) {
                if (!first) {
                    out << "\n";
                }
                first = false;
                out << formatOptionBlock(option);
            }
        }
        return out.str();
    }

    std::string formatDiff(const std::vector<ChangeRecord>& changes) {
        std::ostringstream out;
        if (changes.empty()) {
            out << "No option changes.\n";
            return out.str();
        }
        out << "Changed options:\n";
        for (const ChangeRecord& change : changes) {
            out << "  " << change.fullKey << ": " << change.before << " -> " << change.after << "\n";
        }
        return out.str();
    }

    std::string formatMissingRequired(const std::vector<const ConfigOption*>& missing) {
        std::ostringstream out;
        if (missing.empty()) {
            out << "All required options have a usable value.\n";
            return out.str();
        }
        out << "Missing required options:\n";
        for (const ConfigOption* option : missing) {
            out << "  " << fullOptionKey(*option);
            if (!option->description.empty()) {
                out << " - " << firstLineOf(option->description);
            }
            out << "\n";
        }
        return out.str();
    }

    bool writeFile(const std::string& path, const std::string& content, std::string& error) {
        try {
            const std::filesystem::path filePath(path);
            if (filePath.has_parent_path() && !filePath.parent_path().empty() && !std::filesystem::exists(filePath.parent_path())) {
                std::filesystem::create_directories(filePath.parent_path());
            }
        } catch (const std::filesystem::filesystem_error& exception) {
            error = std::string("Could not create parent directory: ") + exception.what();
            return false;
        }

        std::ofstream file(path, std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            error = "Could not open file for writing: " + path;
            return false;
        }

        file << content;
        if (!file.good()) {
            error = "Failed while writing file: " + path;
            return false;
        }

        return true;
    }

    std::string createTempFile(const std::string& content, std::string& error) {
        std::filesystem::path pathTemplate;
        try {
            pathTemplate = std::filesystem::temp_directory_path() / "snodec-control-XXXXXX";
        } catch (const std::filesystem::filesystem_error& exception) {
            error = exception.what();
            return "";
        }

        std::string templateString = pathTemplate.string();
        std::vector<char> buffer(templateString.begin(), templateString.end());
        buffer.push_back('\0');

        const int fd = ::mkstemp(buffer.data());
        if (fd < 0) {
            error = std::strerror(errno);
            return "";
        }

        const std::string path(buffer.data());
        const ssize_t written = ::write(fd, content.data(), content.size());
        ::close(fd);

        if (written < 0 || static_cast<std::size_t>(written) != content.size()) {
            error = "short write while creating temporary file";
            ::unlink(path.c_str());
            return "";
        }

        return path;
    }

    RunConfigResolution resolveRunConfigPath(const std::optional<std::string>& runConfigPath,
                                              const std::optional<std::string>& saveConfigPath,
                                              const std::optional<std::string>& savedConfigPathForRun,
                                              bool haveEdits,
                                              bool dryRun,
                                              const ConfigModel& model,
                                              const std::string& targetPath) {
        RunConfigResolution resolution;

        if (runConfigPath.has_value()) {
            if (!std::filesystem::exists(*runConfigPath)) {
                resolution.ok = false;
                resolution.error = "run config file does not exist: " + *runConfigPath;
                return resolution;
            }
            resolution.configPath = *runConfigPath;
            return resolution;
        }

        if (saveConfigPath.has_value()) {
            if (savedConfigPathForRun.has_value()) {
                resolution.configPath = *savedConfigPathForRun;
                return resolution;
            }
            if (dryRun) {
                resolution.configPath = *saveConfigPath;
                return resolution;
            }
            resolution.ok = false;
            resolution.error = "cannot run: --save-config did not succeed, so there is no saved config file to run with";
            return resolution;
        }

        if (haveEdits) {
            if (dryRun) {
                resolution.configPath = "<generated-temp-config>";
                return resolution;
            }
            std::string error;
            const std::string tempPath = createTempFile(materialize(model, targetPath), error);
            if (tempPath.empty()) {
                resolution.ok = false;
                resolution.error = "could not create temporary materialized config for --run: " + error;
                return resolution;
            }
            resolution.configPath = tempPath;
            resolution.tempPathCreated = tempPath;
            return resolution;
        }

        return resolution; // no config file at all: run the target as-is
    }

    SaveOutcome performSaveConfig(const ConfigModel& model,
                                  const std::string& targetPath,
                                  const std::vector<std::string>& targetArgTokens,
                                  const std::string& saveConfigPath,
                                  bool dryRun,
                                  bool keepTemp) {
        SaveOutcome outcome;
        const std::string materialized = materialize(model, targetPath);

        if (dryRun) {
            const auto saveArgsOpt = buildSaveArgs(targetArgTokens, "<generated-temp-config>", saveConfigPath);
            if (!saveArgsOpt) {
                outcome.isError = true;
                outcome.message = "Error: --target-args already specifies a config file; refusing to also append "
                                   "save arguments (conflict).\n";
                return outcome;
            }
            outcome.message =
                "[dry-run] Would save config to " + saveConfigPath + " via: " + formatCommandForDisplay(targetPath, *saveArgsOpt) + "\n";
            return outcome;
        }

        std::string tempError;
        const std::string tempPath = createTempFile(materialized, tempError);
        if (tempPath.empty()) {
            outcome.isError = true;
            outcome.message = "Error: could not create temporary materialized config for --save-config: " + tempError + "\n";
            return outcome;
        }

        const auto saveArgsOpt = buildSaveArgs(targetArgTokens, tempPath, saveConfigPath);
        if (!saveArgsOpt) {
            outcome.isError = true;
            outcome.message = "Error: --target-args already specifies a config file; refusing to also append "
                               "save arguments (conflict).\n";
            if (keepTemp) {
                outcome.message += "Kept temporary materialized config at " + tempPath + "\n";
            } else {
                std::filesystem::remove(tempPath);
            }
            return outcome;
        }

        std::error_code existsErrorBefore;
        const bool existedBefore = std::filesystem::exists(saveConfigPath, existsErrorBefore);
        std::filesystem::file_time_type mtimeBefore{};
        if (existedBefore) {
            std::error_code mtimeError;
            mtimeBefore = std::filesystem::last_write_time(saveConfigPath, mtimeError);
        }

        const ProcessResult saveResult = runProcess(targetPath, *saveArgsOpt);

        std::ostringstream tempNote;
        if (keepTemp) {
            tempNote << "Kept temporary materialized config at " << tempPath << "\n";
        } else {
            std::filesystem::remove(tempPath);
        }

        if (!saveResult.spawned) {
            outcome.isError = true;
            outcome.message = "Error: failed to execute target '" + targetPath + "' to save config: " + saveResult.spawnError + "\n" +
                               tempNote.str();
            return outcome;
        }

        std::error_code existsErrorAfter;
        const bool existsAfter = std::filesystem::exists(saveConfigPath, existsErrorAfter);
        bool wroteFile = false;
        if (existsAfter) {
            if (!existedBefore) {
                wroteFile = true;
            } else {
                std::error_code mtimeErrorAfter;
                const auto mtimeAfter = std::filesystem::last_write_time(saveConfigPath, mtimeErrorAfter);
                wroteFile = !mtimeErrorAfter && mtimeAfter != mtimeBefore;
            }
        }

        std::ostringstream message;
        if (wroteFile) {
            if (saveResult.exitCode != 0) {
                message << "Note: target exited with status " << saveResult.exitCode
                        << " while saving config (this is expected for SNode.C applications: writing a "
                           "config file never proceeds to run); the config file was written successfully.\n";
            }
            message << "Saved canonical configuration to " << saveConfigPath << "\n";
            outcome.succeeded = true;
        } else {
            message << "Error: target did not write " << saveConfigPath << " (exit code " << saveResult.exitCode << ").\n";
            if (!saveResult.stdErr.empty()) {
                message << "Target stderr:\n" << saveResult.stdErr;
            }
            if (!saveResult.stdOut.empty()) {
                message << "Target stdout:\n" << saveResult.stdOut;
            }
            outcome.isError = true;
        }
        message << tempNote.str();
        outcome.message = message.str();

        return outcome;
    }

} // namespace snodec::control
