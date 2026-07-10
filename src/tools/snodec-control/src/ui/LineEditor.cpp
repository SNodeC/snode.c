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

#include "LineEditor.h"

#include <utility>

namespace snodec::control::ui {

    LineEditorBuffer::LineEditorBuffer(std::string initial) : buffer(std::move(initial)), cursorPos(buffer.size()) {
    }

    const std::string& LineEditorBuffer::text() const {
        return buffer;
    }

    std::size_t LineEditorBuffer::cursor() const {
        return cursorPos;
    }

    void LineEditorBuffer::insert(char c) {
        buffer.insert(buffer.begin() + static_cast<std::string::difference_type>(cursorPos), c);
        ++cursorPos;
    }

    void LineEditorBuffer::backspace() {
        if (cursorPos == 0) {
            return;
        }
        buffer.erase(buffer.begin() + static_cast<std::string::difference_type>(cursorPos) - 1);
        --cursorPos;
    }

    void LineEditorBuffer::deleteForward() {
        if (cursorPos >= buffer.size()) {
            return;
        }
        buffer.erase(buffer.begin() + static_cast<std::string::difference_type>(cursorPos));
    }

    void LineEditorBuffer::moveLeft() {
        if (cursorPos > 0) {
            --cursorPos;
        }
    }

    void LineEditorBuffer::moveRight() {
        if (cursorPos < buffer.size()) {
            ++cursorPos;
        }
    }

    void LineEditorBuffer::moveToStart() {
        cursorPos = 0;
    }

    void LineEditorBuffer::moveToEnd() {
        cursorPos = buffer.size();
    }

} // namespace snodec::control::ui
