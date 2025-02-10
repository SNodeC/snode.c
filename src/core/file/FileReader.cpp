/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "core/file/FileReader.h"

#include "core/State.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"
#include "log/Logger.h"

#include <cerrno>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

constexpr int MF_READSIZE = 16384;

namespace core::file {
    /*
        FileReader::FileReader(int fd, const std::string& name, std::size_t pufferSize, int openErrno)
            : core::Descriptor(fd)
            , EventReceiver(name)
            , pufferSize(pufferSize)
            , openErrno(openErrno) {
        }

        FileReader* FileReader::open(const std::string& path) {
            errno = 0;

            const int fd = core::system::open(path.c_str(), O_RDONLY);

            return new FileReader(fd, "FileReader: " + path, MF_READSIZE, fd < 0 ? errno : 0);
        }

        bool FileReader::isOpen() {
            return getFd() >= 0;
        }

        void FileReader::onEvent([[maybe_unused]] const utils::Timeval& currentTime) {
            if (running && core::eventLoopState() != core::State::STOPPING) {
                if (!suspended) {
                    std::vector<char> puffer(pufferSize);

                    const ssize_t ret = core::system::read(getFd(), puffer.data(), puffer.capacity());
                    if (ret > 0) {
                        if (this->send(puffer.data(), static_cast<std::size_t>(ret)) < 0) {
                            running = false;

                            this->error(errno);
                        }
                    } else {
                        running = false;

                        if (ret == 0) {
                            this->eof();
                        } else {
                            this->error(errno);
                        }
                    }

                    span();
                }
            } else {
                delete this;
            }
        }
    */

    // =====================================================================
    // Implementation of FileReader::CacheManager and its inner class CachedFile
    // =====================================================================

    FileReader::CacheManager::CacheManager(size_t maxSize)
        : maxCacheSize(maxSize)
        , currentCacheSize(0) {
    }

    const std::string* FileReader::CacheManager::getCachedContent(const std::string& filename, fs::file_time_type& cachedTime) {
        auto it = cache.find(filename);
        if (it != cache.end()) {
            cachedTime = it->second.lastWriteTime;
            const fs::file_time_type currentTime = fs::last_write_time(filename);
            if (it->second.lastWriteTime != currentTime) {
                // The file has changed—remove the outdated entry if not in use.
                if (it->second.refCount == 0) {
                    currentCacheSize -= it->second.content.size();
                    insertionOrder.erase(it->second.orderIt);
                    cache.erase(it);
                }
                return nullptr;
            }
            // Increment the refCount to mark that this entry is in use.
            it->second.refCount++;
            return &(it->second.content);
        }
        return nullptr;
    }

    void FileReader::CacheManager::evictForFile(size_t fileSize) {
        // Evict non‑in‑use entries until there is enough room for the new file.
        // We do not remove an entry if its refCount > 0.
        while (currentCacheSize + fileSize > maxCacheSize && !insertionOrder.empty()) {
            const std::string& oldestKey = insertionOrder.front();
            auto it = cache.find(oldestKey);
            if (it != cache.end() && it->second.refCount == 0) {
                currentCacheSize -= it->second.content.size();
                insertionOrder.pop_front();
                cache.erase(it);
                std::cout << "CacheManager: Evicted cached file \"" << oldestKey << "\" to make room.\n";
            }
        }
    }

    void FileReader::CacheManager::createCacheEntry(const std::string& filename) {
        const fs::file_time_type lastWriteTime = fs::last_write_time(filename);
        insertionOrder.push_back(filename);
        auto listIt = std::prev(insertionOrder.end());
        CachedFile cf;
        cf.content = "";
        cf.orderIt = listIt;
        cf.lastWriteTime = lastWriteTime;
        cf.refCount = 1; // When created, the caller will hold one reference.
        cache.insert({filename, cf});
        // Note: currentCacheSize is not increased here because content is empty.
    }

    std::string* FileReader::CacheManager::getMutableCacheEntry(const std::string& filename) {
        auto it = cache.find(filename);
        if (it != cache.end()) {
            return &(it->second.content);
        }
        return nullptr;
    }

    void FileReader::CacheManager::appendToCacheEntry(size_t appendedSize) {
        // Simply increase the current cache size.
        currentCacheSize += appendedSize;
        // No eviction is performed here.
    }

    void FileReader::CacheManager::releaseCache(const std::string& filename) {
        auto it = cache.find(filename);
        if (it != cache.end()) {
            if (it->second.refCount > 0) {
                it->second.refCount--;
            }
        }
    }

    size_t FileReader::CacheManager::getMaxCacheSize() const {
        return maxCacheSize;
    }

    // =====================================================================
    // End of CacheManager implementation
    // =====================================================================

    // =====================================================================
    // Implementation of FileReader
    // =====================================================================

    // Define the static CacheManager instance (max cache size of 10 KB).
    FileReader::CacheManager FileReader::cacheManager(300 * 1024);

    FileReader::FileReader(const std::string& name,
                           size_t chunkSize,
                           [[maybe_unused]] int openError,
                           std::string* cachedContent,
                           std::unique_ptr<std::ifstream>& fileStream)
        : EventReceiver(name)
        , chunkSize(chunkSize)
        , offset(0)
        , cachedContent(cachedContent)
        , fileStream(std::move(fileStream)) {
    }

    FileReader* FileReader::open(const std::string& fileName) {
        // Determine whether the file is cacheable (its size does not exceed the allowed max).
        bool cacheable = false;
        size_t fsize = 0;
        try {
            fsize = fs::file_size(fileName);
            if (fsize <= cacheManager.getMaxCacheSize()) {
                cacheable = true;
            }
        } catch (...) {
            cacheable = false;
        }

        std::string* cachedContent = nullptr;
        // For incremental reading when no valid cached content exists.
        std::unique_ptr<std::ifstream> fileStream;

        fs::file_time_type dummy;
        const std::string* cached = cacheManager.getCachedContent(fileName, dummy);
        if (cached != nullptr) {
            // Up-to-date cached content exists.
            // (We cast away const–ness because we plan to update the entry as new chunks arrive.)
            cachedContent = const_cast<std::string*>(cached);
            LOG(INFO) << "FileReader: Using cached content for \"" << fileName << "\".\n";
        } else if (cacheable) {
            // Before creating a new cache entry, evict non-in-use entries (if any) so that
            // there is room for the new file.
            cacheManager.evictForFile(fsize);
            // Create an empty cache entry.
            cacheManager.createCacheEntry(fileName);
            cachedContent = cacheManager.getMutableCacheEntry(fileName);
            fileStream = std::make_unique<std::ifstream>(fileName, std::ios::binary);
            if (!fileStream || !fileStream->is_open()) {
                throw std::runtime_error("FileReader: Cannot open file: " + fileName);
            }
            LOG(INFO) << "FileReader: Reading file incrementally and caching for \"" << fileName << "\".\n";
        } else {
            // File is not cacheable; open for incremental reading without caching.
            fileStream = std::make_unique<std::ifstream>(fileName, std::ios::binary);
            LOG(INFO) << "FileReader: Reading file incrementally (not cacheable) for \"" << fileName << "\".\n";
        }

        return new FileReader("FileReader: " + fileName, MF_READSIZE, !fileStream->is_open() ? errno : 0, cachedContent, fileStream);
    }

    bool FileReader::isOpen() {
        return cachedContent != nullptr || fileStream->is_open();
    }

    void FileReader::onEvent([[maybe_unused]] const utils::Timeval& currentTime) {
        if (running && core::eventLoopState() != core::State::STOPPING) {
            if (!suspended) {
                if (cachedContent != nullptr && fileStream == nullptr) {
                    // Entire file content is cached—send from memory.
                    if (offset >= cachedContent->size()) {
                        running = false;

                        this->eof();
                    }
                    const size_t remaining = cachedContent->size() - offset;
                    const size_t currentChunk = std::min(chunkSize, remaining);
                    this->send(cachedContent->data() + offset, currentChunk);
                    offset += currentChunk;
                    span();
                } else if (fileStream) {
                    std::vector<char> buffer(chunkSize);
                    fileStream->read(buffer.data(), static_cast<ssize_t>(chunkSize));
                    const std::streamsize bytesRead = fileStream->gcount();
                    if (bytesRead > 0) {
                        // If caching is enabled, append this chunk directly into the cache entry.
                        if (cachedContent != nullptr) {
                            cachedContent->append(buffer.data(), static_cast<size_t>(bytesRead));
                            cacheManager.appendToCacheEntry(static_cast<size_t>(bytesRead));
                        }
                        this->send(buffer.data(), static_cast<size_t>(bytesRead));
                        span();
                    } else if (fileStream->eof()) {
                        fileStream->close();
                        fileStream.reset();

                        running = false;
                        this->eof();
                    } else {
                        fileStream->close();
                        fileStream.reset();
                        this->error(errno);
                    }
                }
            }
        }
    }

    bool FileReader::isFinished() const {
        if (cachedContent != nullptr && fileStream == nullptr) {
            return offset >= cachedContent->size();
        }

        if (fileStream) {
            return fileStream->eof();
        }

        return true;
    }

    FileReader::~FileReader() {
        // If this FileReader was using a cached entry, release it.
        if (cachedContent != nullptr) {
            cacheManager.releaseCache(filename);
        }
    }

    void FileReader::start() {
        if (!running) {
            running = true;
            span();
        }
    }

    void FileReader::suspend() {
        if (running) {
            suspended = true;
        }
    }

    void FileReader::resume() {
        if (running) {
            suspended = false;
            span();
        }
    }

    void FileReader::stop() {
        if (running) {
            this->eof();

            running = false;
            span();
        }
    }

} // namespace core::file
