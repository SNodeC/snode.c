/*
 * SNode.C - A Slim Toolkit for Network Communication
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

#include "support/TestResult.h"
#include "utils/SubCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <memory>
#include <string_view>
#include <utility>
#endif

namespace {

    class Root final : public utils::SubCommand {
    public:
        Root()
            : SubCommand(nullptr, std::make_shared<utils::AppWithPtr>("root", "root", this), "Tests") {
        }
    };

    class Forwarded final : public utils::SubCommand {
    public:
        static constexpr std::string_view NAME = "forwarded";
        static constexpr std::string_view DESCRIPTION = "Forwarded construction arguments";

        Forwarded(utils::SubCommand* parent, int& reference, std::unique_ptr<int> owned)
            : SubCommand(parent, this, "Tests")
            , reference(reference)
            , owned(std::move(owned)) {
        }

        int& reference;
        std::unique_ptr<int> owned;
    };

} // namespace

int main() {
    tests::support::TestResult test;
    Root root;
    int reference = 7;
    auto owned = std::make_unique<int>(42);
    const auto* child = root.newSubCommand<Forwarded>(reference, std::move(owned));
    test.expectTrue(child != nullptr && owned == nullptr, "factory accepts and transfers a move-only argument");
    if (child != nullptr) {
        test.expectTrue(&child->reference == &reference && *child->owned == 42, "factory preserves lvalue identity and owned value");
        child->reference = 9;
        test.expectTrue(reference == 9, "forwarding does not copy an lvalue argument");
    }
    return test.processResult();
}
