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

#ifndef SNODECCONTROL_CONFIGEDITOR_H
#define SNODECCONTROL_CONFIGEDITOR_H

#include "ConfigModel.h"

#include <string>
#include <vector>

namespace snodec::control {

    enum class LookupStatus { Found, NotFound, Ambiguous };

    struct LookupResult {
        LookupStatus status = LookupStatus::NotFound;
        ConfigOption* option = nullptr;      // valid only when status == Found
        std::vector<std::string> candidates; // full keys of matching options when status == Ambiguous
    };

    // Looks up an option by its fully qualified key (exact match against section + "." + key, or just
    // the bare key for the root/global section) or, failing that, by its bare trailing key component
    // across all sections. A bare-key match that hits more than one section is reported as Ambiguous
    // with all matching full keys listed as candidates.
    LookupResult findOption(ConfigModel& model, const std::string& key);

    // Returns the human-readable "effective" value of an option: its active value if present, else its
    // default value if present, else "<REQUIRED>" for a required option, else "<unset>". Empty string
    // values (active or default) are rendered as "" so they are visibly distinct from "no value at
    // all". Used by --list-options, --get, and as the "before"/"after" snapshots for --diff.
    std::string effectiveValueDisplay(const ConfigOption& option);

    // A single --set/--unset edit as parsed from the command line, applied to the model in argv order.
    struct EditOperation {
        enum class Kind { Set, Unset };

        Kind kind = Kind::Set;
        std::string key;
        std::string value; // only meaningful for Kind::Set
    };

    // Parses a "key=value" --set argument, splitting only at the first '='. An empty value after '=' is
    // valid (e.g. "name="). Returns false, leaving key/value untouched, if `argument` contains no '='.
    bool parseSetArgument(const std::string& argument, std::string& key, std::string& value);

    // A single recorded, effective change for --diff reporting. Only produced when an edit actually
    // altered the option's effective value/state; a --set to the value it already had, or an --unset of
    // an option with no active value, produce no ChangeRecord.
    struct ChangeRecord {
        std::string fullKey;
        std::string before;
        std::string after;
    };

    struct EditError {
        std::string key;
        std::string message;
    };

    struct EditOutcome {
        std::vector<ChangeRecord> changes;
        std::vector<EditError> errors;
    };

    // Applies `operations` to `model` in order. Unknown keys are an error unless `allowNewOptions` is
    // set, in which case a --set for an unknown key creates a new option (section/key derived from the
    // dotted key exactly like discovery does); --unset for an unknown key still creates it if
    // `allowNewOptions` is set, but since a freshly created option has no active value to remove, that
    // is always a no-op producing no ChangeRecord. Ambiguous bare-key references are always an error,
    // regardless of `allowNewOptions`.
    EditOutcome applyEdits(ConfigModel& model, const std::vector<EditOperation>& operations, bool allowNewOptions);

    // True if `option` is marked required but has neither a usable active value nor a usable default
    // (i.e. would still display as "<REQUIRED>"). The per-option predicate behind missingRequiredOptions()
    // and --check-required, also used by the UI to render a required-but-unset marker next to an option.
    bool isOptionMissingRequired(const ConfigOption& option);

    // Options whose required marker is set but which, after any edits, have neither a usable active
    // value nor a usable default (i.e. would still display as "<REQUIRED>"). Used by --check-required.
    std::vector<const ConfigOption*> missingRequiredOptions(const ConfigModel& model);

    // Merges two ordered sequences of ChangeRecords produced by separate applyEdits() calls against the
    // same model over time (e.g. initial --set/--unset edits from the command line, followed by further
    // interactive edits made in the UI), collapsing multiple entries for the same key into one showing
    // the earliest "before" and the latest "after". An entry whose before/after end up identical (the
    // combined edits net out to no change) is dropped, matching applyEdits()'s own no-op behavior. Used
    // to combine pre-UI and in-UI changes into one coherent --diff after --ui.
    std::vector<ChangeRecord> mergeChangeRecords(const std::vector<ChangeRecord>& earlier, const std::vector<ChangeRecord>& later);

} // namespace snodec::control

#endif // SNODECCONTROL_CONFIGEDITOR_H
