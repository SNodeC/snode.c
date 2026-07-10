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

#include "TestResult.h"
#include "ui/RenderUtil.h"

#include <string>

using snodec::control::ui::fitToWidth;

namespace {

    void testPadsShortText(snodec::control::test::TestResult& testResult) {
        const std::string result = fitToWidth("hi", 5);
        testResult.expectEqual(5, static_cast<int>(result.size()), "pad: result is exactly the requested width");
        testResult.expectEqual(std::string("hi   "), result, "pad: short text is space-padded on the right");
    }

    void testTruncatesLongText(snodec::control::test::TestResult& testResult) {
        const std::string result = fitToWidth("hello world", 5);
        testResult.expectEqual(5, static_cast<int>(result.size()), "truncate: result is exactly the requested width");
        testResult.expectEqual(std::string("hello"), result, "truncate: long text is cut off at the width, not wrapped");
    }

    void testExactWidthIsUnchanged(snodec::control::test::TestResult& testResult) {
        const std::string result = fitToWidth("exact", 5);
        testResult.expectEqual(std::string("exact"), result, "exact: text exactly the target width passes through unchanged");
    }

    void testZeroOrNegativeWidth(snodec::control::test::TestResult& testResult) {
        testResult.expectEqual(std::string(""), fitToWidth("anything", 0), "zero width: always returns an empty string");
        testResult.expectEqual(std::string(""), fitToWidth("anything", -5), "negative width: always returns an empty string, never crashes");
    }

    void testEmptyInput(snodec::control::test::TestResult& testResult) {
        const std::string result = fitToWidth("", 4);
        testResult.expectEqual(std::string("    "), result, "empty input: pads out to the full requested width");
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testPadsShortText(testResult);
    testTruncatesLongText(testResult);
    testExactWidthIsUnchanged(testResult);
    testZeroOrNegativeWidth(testResult);
    testEmptyInput(testResult);

    return testResult.processResult();
}
