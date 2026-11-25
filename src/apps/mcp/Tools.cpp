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

#include "Tools.h"

#include "core/system/unistd.h"
#include "core/timer/Timer.h"
#include "mcp-config.h"
#include "mcp/server/Config.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Server.h"
#include "mcp/server/Types.h"
#include "utils/CLI11.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <deque>
#include <dirent.h>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace apps::mcp {

    using ::mcp::server::Json;
    using ::mcp::server::ToolReply;
    using ::mcp::server::ToolResult;

    bool header(const std::string& path) {
        return path.ends_with(".h") || path.ends_with(".hpp");
    }

    class Headers {
    public:
        explicit Headers(const std::string& path)
            : root(::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW)) {
            if (root < 0) {
                throw std::runtime_error("Cannot open installed header root");
            }
        }

        Headers(const Headers&) = delete;
        Headers& operator=(const Headers&) = delete;

        ~Headers() {
            ::close(root);
        }

        int open(const std::string& path, bool directory) const {
            std::vector<std::string> parts;
            std::size_t position = 0;
            bool valid = path.size() <= 512 && path.find('\0') == std::string::npos && (directory || header(path));
            while (valid && position < path.size()) {
                const auto slash = path.find('/', position);
                const std::string part = path.substr(position, slash == std::string::npos ? slash : slash - position);
                valid = !part.empty() && part != "." && part != ".." && parts.size() < 16;
                parts.push_back(part);
                position = slash == std::string::npos ? path.size() : slash + 1;
            }

            valid = valid && (path.empty() || path.back() != '/');
            int fd = valid ? ::openat(root, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW) : -1;
            for (std::size_t i = 0; fd >= 0 && i < parts.size(); ++i) {
                const bool parent = directory || i + 1 < parts.size();
                const int next = ::openat(fd, parts[i].c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | (parent ? O_DIRECTORY : O_NONBLOCK));
                ::close(fd);
                fd = next;
            }

            struct stat info{};
            if (fd >= 0 && !directory && (::fstat(fd, &info) != 0 || !S_ISREG(info.st_mode))) {
                ::close(fd);
                fd = -1;
            }

            return fd;
        }

    private:
        int root;
    };

    namespace {

        class Lookup final : public std::enable_shared_from_this<Lookup> {
        public:
            Lookup(std::shared_ptr<Headers> headers,
                   std::shared_ptr<ToolReply> reply,
                   Json args,
                   LookupMode mode,
                   std::size_t maximumFiles,
                   std::size_t maximumBytes)
                : headers(std::move(headers))
                , reply(std::move(reply))
                , args(std::move(args))
                , mode(mode)
                , maximumFiles(maximumFiles)
                , maximumBytes(mode == LookupMode::Read ? 65536 : maximumBytes) {
                limit =
                    this->args.value(mode == LookupMode::Read ? "bytes" : "limit", std::size_t{mode == LookupMode::Read ? 16384U : 20U});
                query = this->args.value("query", std::string());
            }

            ~Lookup();

            void start(const ::mcp::server::RequestContext& context) {
                const std::weak_ptr<Lookup> weak = shared_from_this();
                context.onCancel([weak](::mcp::server::CancelReason) {
                    if (const auto current = weak.lock()) {
                        current->cancel();
                    }
                });
                schedule();
            }

        private:
            void schedule() {
                timer = core::timer::Timer::singleshotTimer(
                    [self = shared_from_this()] {
                        auto current = std::move(self->timer); // Detach this firing handle before scheduling its successor.
                        self->tick();
                    },
                    utils::Timeval(0.001));
            }

            void cancel() {
                done = true;
                timer.cancel();
                closeFile();
                if (directory != nullptr) {
                    ::closedir(directory);
                    directory = nullptr;
                }
            }

            core::timer::Timer timer;
            std::shared_ptr<Headers> headers;
            std::shared_ptr<ToolReply> reply;
            Json args;
            LookupMode mode;
            std::size_t maximumFiles;
            std::size_t maximumBytes;
            std::size_t limit = 0;
            std::size_t files = 0;
            std::size_t bytes = 0;
            std::size_t entries = 0;
            std::size_t offset = 0;
            std::size_t lines = 0;
            std::string query;
            std::string path;
            std::string folder;
            std::string tail;
            std::string text;
            std::string failure;
            std::deque<std::string> directories{std::string()};
            DIR* directory = nullptr;
            int file = -1;
            Json matches = Json::array();
            bool done = false;
            bool truncated = false;

            void collect(const char* chunk, std::size_t length) {
                if (!done) {
                    bytes += length;
                    if (mode == LookupMode::Read) {
                        text.append(chunk, std::min(length, limit - text.size()));
                        done = text.size() == limit;
                        truncated = done;
                    } else {
                        const std::string window = tail + std::string(chunk, length);
                        const std::size_t baseLine = lines - static_cast<std::size_t>(std::count(tail.begin(), tail.end(), '\n')) + 1;
                        std::size_t at = window.find(query);
                        while (at != std::string::npos && matches.size() < limit) {
                            matches.push_back({{"path", path},
                                               {"offset", offset - tail.size() + at},
                                               {"line",
                                                baseLine + static_cast<std::size_t>(std::count(
                                                               window.begin(), window.begin() + static_cast<std::ptrdiff_t>(at), '\n'))}});
                            at = window.find(query, at + 1);
                        }

                        lines += static_cast<std::size_t>(std::count(chunk, chunk + length, '\n'));
                        offset += length;
                        tail = window.substr(window.size() - std::min(window.size(), query.size() - 1));
                        done = matches.size() == limit;
                        truncated = done;
                    }
                }
            }

            void closeFile() {
                if (file >= 0) {
                    ::close(file);
                    file = -1;
                }
            }

            void readChunk() {
                std::array<char, 16384> chunk{};
                const std::size_t remaining = mode == LookupMode::Read ? limit - text.size() : maximumBytes - bytes;
                const ssize_t count = core::system::read(file, chunk.data(), std::min(chunk.size(), remaining));
                if (count > 0) {
                    collect(chunk.data(), static_cast<std::size_t>(count));
                } else if (count == 0) {
                    closeFile();
                    if (mode == LookupMode::Read) {
                        done = true;
                        truncated = false;
                    }
                } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                    failure = "Header read failed";
                    done = true;
                }
            }

            void attach() {
                int fd = headers->open(path, false);
                if (fd >= 0 && mode == LookupMode::Read &&
                    ::lseek(fd, static_cast<off_t>(args.value("offset", std::size_t{0})), SEEK_SET) < 0) {
                    ::close(fd);
                    fd = -1;
                }

                if (fd >= 0) {
                    ++files;
                    tail.clear();
                    offset = 0;
                    lines = 0;
                    file = fd;
                } else if (mode == LookupMode::Read) {
                    failure = "Header is unavailable or outside the confined tree";
                    done = true;
                }
            }

            void traverse() {
                if (directory == nullptr && !directories.empty()) {
                    folder = std::move(directories.front());
                    directories.pop_front();
                    const int fd = headers->open(folder, true);
                    if (fd >= 0) {
                        directory = ::fdopendir(fd);
                        if (directory == nullptr) {
                            ::close(fd);
                        }
                    }
                }

                for (std::size_t i = 0; i < 8 && directory != nullptr && file < 0 && !done; ++i) {
                    dirent* entry = ::readdir(directory);
                    if (entry == nullptr) {
                        ::closedir(directory);
                        directory = nullptr;
                    } else {
                        ++entries;
                        const std::string name(entry->d_name);
                        const std::string candidate = folder.empty() ? name : folder + "/" + name;
                        struct stat info{};
                        if (name != "." && name != ".." && candidate.size() <= 512 &&
                            ::fstatat(::dirfd(directory), name.c_str(), &info, AT_SYMLINK_NOFOLLOW) == 0) {
                            static_cast<void>(Json(candidate).dump());
                            if (S_ISDIR(info.st_mode) && directories.size() < 512) {
                                directories.push_back(candidate);
                            } else if (S_ISREG(info.st_mode) && header(candidate)) {
                                path = candidate;
                                attach();
                            }
                        }

                        if (entries >= 8192 || directories.size() >= 512) {
                            truncated = true;
                            done = true;
                        }
                    }
                }

                done = done || (directory == nullptr && directories.empty() && file < 0);
            }

            void finish() {
                closeFile();
                if (!failure.empty()) {
                    reply->result(ToolResult::toolError(failure));
                } else if (mode == LookupMode::Read) {
                    const Json value{{"path", path},
                                     {"offset", args.value("offset", std::size_t{0})},
                                     {"bytes", text.size()},
                                     {"text", text},
                                     {"truncated", truncated}};
                    reply->result(ToolResult::json(value));
                } else {
                    const Json value{
                        {"matches", matches}, {"files", files}, {"bytes", bytes}, {"entries", entries}, {"truncated", truncated}};
                    reply->result(ToolResult::json(value));
                }
            }

            void tick() {
                try {
                    if (!done && (bytes >= maximumBytes || (files >= maximumFiles && file < 0)) && mode == LookupMode::Find) {
                        truncated = true;
                        done = true;
                    }

                    if (!done && file >= 0) {
                        readChunk();
                    } else if (!done && mode == LookupMode::Read) {
                        path = args["path"].get<std::string>();
                        attach();
                    } else if (!done) {
                        traverse();
                    }

                    if (done) {
                        finish();
                    } else {
                        schedule();
                    }
                } catch (const std::exception&) {
                    failure = "Lookup failed (including invalid UTF-8 header data)";
                    finish();
                }
            }
        };

        Lookup::~Lookup() {
            closeFile();
            if (directory != nullptr) {
                ::closedir(directory);
            }
        }

        constexpr const char* style = "SNode.C: use one event loop; configure named instances before init; listen, then start. "
                                      "Use semantic logging, express routing and existing CLI11 instance options. "
                                      "Keep protocol code independent of transports. Bound every handler's work. "
                                      "Stream with Response::pipe and finish through Source EOF. Never mix send/json with an open stream.";

    } // namespace

    DevelopmentConfig::DevelopmentConfig(utils::SubCommand* parent)
        : ::mcp::server::Config(parent) {
        headerRoot = addOption("--header-root", "Installed SNode.C header root", "directory", CLI::ExistingDirectory);
        setDefaultValue(headerRoot, std::string(MCP_HEADER_ROOT));
        searchFiles = addOption("--search-files", "Maximum headers per search", "count", std::size_t{4096}, CLI::Range(1, 8192));
        searchBytes = addOption("--search-bytes", "Maximum bytes read per search", "bytes", std::size_t{8388608}, CLI::Range(1, 67108864));
    }

    std::string DevelopmentConfig::getHeaderRoot() const {
        return headerRoot->as<std::string>();
    }

    std::size_t DevelopmentConfig::getSearchFiles() const {
        return searchFiles->as<std::size_t>();
    }

    std::size_t DevelopmentConfig::getSearchBytes() const {
        return searchBytes->as<std::size_t>();
    }

    std::string instructions() {
        return style;
    }

    std::shared_ptr<Headers> openHeaders(const std::string& root) {
        return std::make_shared<Headers>(root);
    }

    void lookup(const std::shared_ptr<Headers>& headers,
                const ::mcp::server::ToolCall& call,
                const std::shared_ptr<::mcp::server::ToolReply>& reply,
                LookupMode mode,
                std::size_t files,
                std::size_t bytes) {
        std::make_shared<Lookup>(headers, reply, call.arguments, mode, files, bytes)->start(call.context);
    }

    ::mcp::server::Json parseSchema(const std::string& name, const char* text, const char* field) {
        ::mcp::server::Json schema;
        try {
            schema = nlohmann::ordered_json::parse(text);
        } catch (const ::mcp::server::Json::parse_error& error) {
            throw ::mcp::server::RegistrationError(::mcp::server::RegistrationKind::Tool, name, field, error.what());
        }

        return schema;
    }

} // namespace apps::mcp
