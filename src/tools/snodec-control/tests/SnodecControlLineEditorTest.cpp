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
#include "ui/LineEditor.h"

#include <string>

using snodec::control::ui::LineEditorBuffer;

namespace {

    void testInitialState(snodec::control::test::TestResult& testResult) {
        LineEditorBuffer empty;
        testResult.expectEqual(std::string(""), empty.text(), "initial: default buffer is empty");
        testResult.expectEqual(0, static_cast<int>(empty.cursor()), "initial: default cursor is at position 0");

        LineEditorBuffer prefilled("hello");
        testResult.expectEqual(std::string("hello"), prefilled.text(), "initial: prefilled buffer keeps its text");
        testResult.expectEqual(5, static_cast<int>(prefilled.cursor()), "initial: cursor starts at the end of prefilled text");
    }

    void testInsertAtCursor(snodec::control::test::TestResult& testResult) {
        LineEditorBuffer editor("ac");
        editor.moveToStart();
        editor.moveRight(); // cursor now between 'a' and 'c'
        editor.insert('b');

        testResult.expectEqual(std::string("abc"), editor.text(), "insert: inserts at the cursor, not just appends");
        testResult.expectEqual(2, static_cast<int>(editor.cursor()), "insert: cursor advances past the inserted character");
    }

    void testInsertOnEmptyBuffer(snodec::control::test::TestResult& testResult) {
        LineEditorBuffer editor;
        editor.insert('x');
        editor.insert('y');
        testResult.expectEqual(std::string("xy"), editor.text(), "insert: works from an empty buffer, appending in order");
    }

    void testBackspace(snodec::control::test::TestResult& testResult) {
        LineEditorBuffer editor("abc");
        editor.backspace(); // deletes 'c' (cursor was at the end)
        testResult.expectEqual(std::string("ab"), editor.text(), "backspace: deletes the character before the cursor");
        testResult.expectEqual(2, static_cast<int>(editor.cursor()), "backspace: cursor moves back with the deletion");

        editor.moveToStart();
        editor.backspace(); // no-op: cursor is already at position 0
        testResult.expectEqual(std::string("ab"), editor.text(), "backspace: a no-op at the start of the buffer changes nothing");
        testResult.expectEqual(0, static_cast<int>(editor.cursor()), "backspace: cursor stays at 0 when there is nothing before it");
    }

    void testDeleteForward(snodec::control::test::TestResult& testResult) {
        LineEditorBuffer editor("abc");
        editor.moveToStart();
        editor.deleteForward(); // deletes 'a', cursor position unchanged

        testResult.expectEqual(std::string("bc"), editor.text(), "delete: deletes the character at the cursor");
        testResult.expectEqual(0, static_cast<int>(editor.cursor()), "delete: cursor position itself does not move");

        editor.moveToEnd();
        editor.deleteForward(); // no-op: cursor is at the end, nothing after it
        testResult.expectEqual(std::string("bc"), editor.text(), "delete: a no-op at the end of the buffer changes nothing");
    }

    void testCursorMovement(snodec::control::test::TestResult& testResult) {
        LineEditorBuffer editor("abcd");
        testResult.expectEqual(4, static_cast<int>(editor.cursor()), "movement: starts at the end");

        editor.moveLeft();
        editor.moveLeft();
        testResult.expectEqual(2, static_cast<int>(editor.cursor()), "movement: Left moves the cursor back one at a time");

        editor.moveToStart();
        testResult.expectEqual(0, static_cast<int>(editor.cursor()), "movement: Home moves to position 0");

        editor.moveLeft(); // no-op: already at the start
        testResult.expectEqual(0, static_cast<int>(editor.cursor()), "movement: Left at position 0 is a no-op, not negative");

        editor.moveToEnd();
        testResult.expectEqual(4, static_cast<int>(editor.cursor()), "movement: End moves to the end");

        editor.moveRight(); // no-op: already at the end
        testResult.expectEqual(4, static_cast<int>(editor.cursor()), "movement: Right at the end is a no-op, not past the end");
    }

    void testAcceptsEmptyStringAsValid(snodec::control::test::TestResult& testResult) {
        LineEditorBuffer editor("x");
        editor.moveToEnd();
        editor.backspace();
        testResult.expectEqual(std::string(""), editor.text(), "empty: backspacing the only character leaves a valid empty buffer");
        testResult.expectEqual(0, static_cast<int>(editor.cursor()), "empty: cursor is at 0 for an empty buffer");
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testInitialState(testResult);
    testInsertAtCursor(testResult);
    testInsertOnEmptyBuffer(testResult);
    testBackspace(testResult);
    testDeleteForward(testResult);
    testCursorMovement(testResult);
    testAcceptsEmptyStringAsValid(testResult);

    return testResult.processResult();
}
