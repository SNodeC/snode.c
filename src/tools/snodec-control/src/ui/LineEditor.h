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

#ifndef SNODECCONTROL_UI_LINEEDITOR_H
#define SNODECCONTROL_UI_LINEEDITOR_H

#include <cstddef>
#include <string>

namespace snodec::control::ui {

    // A minimal single-line text-editing buffer with a cursor, factored out of the Curses prompt so its
    // editing semantics (insert-at-cursor, Home/End, Backspace/Delete) can be unit tested without a real
    // terminal. Curses-free; CursesUi.cpp's promptText() is the only caller, translating key codes into
    // calls on this buffer and rendering `text()`/`cursor()` itself.
    class LineEditorBuffer {
    public:
        explicit LineEditorBuffer(std::string initial = "");

        const std::string& text() const;
        std::size_t cursor() const;

        // Inserts `c` at the cursor position and advances the cursor past it.
        void insert(char c);

        // Deletes the character immediately before the cursor, if any, and moves the cursor back one.
        void backspace();

        // Deletes the character at the cursor position, if any; the cursor position itself is unchanged.
        void deleteForward();

        void moveLeft();
        void moveRight();
        void moveToStart();
        void moveToEnd();

    private:
        std::string buffer;
        std::size_t cursorPos = 0;
    };

} // namespace snodec::control::ui

#endif // SNODECCONTROL_UI_LINEEDITOR_H
