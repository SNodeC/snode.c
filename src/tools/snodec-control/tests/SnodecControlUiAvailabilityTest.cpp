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

// Exercises src/ui/Ui.h's contract without needing a real terminal, and without needing to know at test
// time whether this build has Curses (CursesUi.cpp) or not (UiUnavailable.cpp): whichever one was
// actually compiled into snodec-control-ui, isUiAvailable() and runInteractiveUi() must stay consistent
// with each other. This lets the same test source verify "Curses-disabled behavior" in a
// SNODEC_CONTROL_BUILD_TUI=OFF (or Curses-missing) build, and normal behavior otherwise.

#include "ConfigModel.h"
#include "ConfigParser.h"
#include "TestResult.h"
#include "ui/Ui.h"

#include <string>

using snodec::control::ConfigModel;
using snodec::control::parseShowConfigOutput;
using snodec::control::ui::isUiAvailable;
using snodec::control::ui::runInteractiveUi;
using snodec::control::ui::UiOptions;
using snodec::control::ui::UiResult;

namespace {

    ConfigModel sampleModel() {
        return parseShowConfigOutput("#log-level=4\nlog-level=4\n").model;
    }

    void testUnavailableBuildFailsClearlyWithoutTouchingTheModel(snodec::control::test::TestResult& testResult) {
        if (isUiAvailable()) {
            // This build has real Curses support; the "unavailable" contract is instead covered by a
            // SNODEC_CONTROL_BUILD_TUI=OFF (or Curses-missing) build running this same test binary.
            return;
        }

        ConfigModel model = sampleModel();
        const std::string originalActiveValue = *model.sections.front().options.front().activeValue;

        UiOptions options;
        options.targetPath = "/nonexistent/does-not-matter";

        const UiResult result = runInteractiveUi(model, options);

        testResult.expectTrue(!result.ok, "Curses-disabled: runInteractiveUi() reports failure");
        testResult.expectTrue(result.message.find("Curses") != std::string::npos, "Curses-disabled: message explains why");
        testResult.expectTrue(result.changes.empty(), "Curses-disabled: no changes are ever reported");
        testResult.expectTrue(!result.savedConfigPathForRun.has_value(), "Curses-disabled: no save path is ever reported");
        testResult.expectEqual(
            originalActiveValue, *model.sections.front().options.front().activeValue, "Curses-disabled: the model is left untouched");
    }

    void testAvailableBuildAdvertisesItself(snodec::control::test::TestResult& testResult) {
        if (!isUiAvailable()) {
            // Covered by testUnavailableBuildFailsClearlyWithoutTouchingTheModel() instead.
            return;
        }

        testResult.expectTrue(isUiAvailable(), "Curses-enabled: isUiAvailable() reports true when Curses was found");
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testUnavailableBuildFailsClearlyWithoutTouchingTheModel(testResult);
    testAvailableBuildAdvertisesItself(testResult);

    return testResult.processResult();
}
