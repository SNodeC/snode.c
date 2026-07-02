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

#include "net/un/SocketAddress.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main() {
    tests::support::TestResult testResult;

    const std::string emptyPath;
    const std::string path = "/tmp/snodec-test.sock";
    const std::string alternatePath = "/tmp/snodec-test-alt.sock";

    const net::un::SocketAddress defaultSocketAddress;
    testResult.expectTrue(defaultSocketAddress.getSunPath() == emptyPath, "default Unix-domain path is empty");
    testResult.expectTrue(defaultSocketAddress.toString() == "@", "default Unix-domain address string is abstract-empty marker");
    testResult.expectTrue(defaultSocketAddress.toString(false) == "@",
                          "compact default Unix-domain address string is abstract-empty marker");

    net::un::SocketAddress pathSocketAddress(path);
    testResult.expectTrue(pathSocketAddress.getSunPath() == path, "Unix-domain path constructor stores path");
    testResult.expectTrue(pathSocketAddress.toString() == "@", "Unix-domain string remains uninitialized until init is called");

    pathSocketAddress.setSunPath(alternatePath);
    testResult.expectTrue(pathSocketAddress.getSunPath() == alternatePath, "Unix-domain setSunPath updates path");
    testResult.expectTrue(pathSocketAddress.toString() == "@", "Unix-domain string still remains uninitialized after setSunPath only");

    const int result = testResult.processResult();

    return result;
}
