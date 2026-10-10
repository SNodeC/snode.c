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
#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {
    std::string racedDirectory;
    bool raceInjected = false;
} // namespace

// Force a competing creator to win precisely after the filesystem precheck.
// All other mkdir calls use libc, as in the existing address-resolution tests.
extern "C" int mkdir(const char* path, mode_t mode) noexcept {
    static const auto real = reinterpret_cast<decltype(&mkdir)>(dlsym(RTLD_NEXT, "mkdir"));
    if (!raceInjected && path == racedDirectory && real(path, 0700) == 0) {
        raceInjected = true;
        errno = EEXIST;
        return -1;
    }
    return real(path, mode);
}

int main(int argc, char* argv[]) {
    if (geteuid() == 0) {
        std::cout << "SKIP: directory initialization requires an unprivileged user and isolated XDG paths\n";
        return tests::support::cTestSkipReturnCode;
    }
    tests::support::TestResult result;
    if (argc != 2) {
        return EXIT_FAILURE;
    }
    const std::string scenario = argv[1];
    char temporary[] = "/tmp/snodec-directory-init-XXXXXX";
    if (mkdtemp(temporary) == nullptr) {
        return EXIT_FAILURE;
    }
    const std::filesystem::path root(temporary);
    const std::string homes[] = {"config", "state", "runtime"};
    const char* variables[] = {"XDG_CONFIG_HOME", "XDG_STATE_HOME", "XDG_RUNTIME_DIR"};
    for (int index = 0; index < 3; ++index) {
        const std::string home = (root / homes[index]).string();
        setenv(variables[index], home.c_str(), 1);
        const auto directory = root / homes[index] / "snode.c";
        if (scenario == "race-" + homes[index]) {
            racedDirectory = directory.string();
        } else if (scenario == "file-" + homes[index]) {
            std::filesystem::create_directories(directory.parent_path());
            std::ofstream(directory) << "not a directory\n";
        } else if (scenario == "existing") {
            std::filesystem::create_directories(directory);
            std::filesystem::permissions(directory, std::filesystem::perms::owner_all);
        }
    }
    char name[] = "directory-initialization-test";
    char* initArguments[] = {name, nullptr};
    bool initialized = false;
    bool filesystemError = false;
    try {
        initialized = utils::Config::init(1, initArguments);
    } catch (const std::filesystem::filesystem_error&) {
        filesystemError = true;
    }
    const bool blocked = scenario.starts_with("file-");
    result.expectTrue(blocked ? filesystemError && !initialized : initialized && !filesystemError,
                      "initialization accepts directories, including a competing creator, but rejects files");
    if (!racedDirectory.empty()) {
        result.expectTrue(raceInjected, "the competing creator won before mkdir");
        result.expectTrue(std::filesystem::status(racedDirectory).permissions() == std::filesystem::perms::owner_all,
                          "initialization preserves the competing creator's permissions");
    }
    if (!blocked) {
        const std::filesystem::perms expected[] = {std::filesystem::perms::owner_all | std::filesystem::perms::group_read |
                                                       std::filesystem::perms::group_exec,
                                                   std::filesystem::perms::owner_all | std::filesystem::perms::group_all,
                                                   std::filesystem::perms::owner_all};
        for (int index = 0; index < 3; ++index) {
            const auto directory = root / homes[index] / "snode.c";
            result.expectTrue(std::filesystem::is_directory(directory), "every initialization path is a directory");
            const auto permissions =
                scenario == "existing" || directory.string() == racedDirectory ? std::filesystem::perms::owner_all : expected[index];
            result.expectTrue(std::filesystem::status(directory).permissions() == permissions,
                              "only newly created directories receive initialization permissions");
        }
    }
    std::filesystem::remove_all(root);
    return result.processResult();
}
