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

#ifndef CORE_FILE_FILEREADER_H
#define CORE_FILE_FILEREADER_H

#include "core/EventReceiver.h"
#include "core/file/File.h"
#include "core/pipe/Source.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::file {

    /*

    class FileReader
        : public core::EventReceiver
        , public core::pipe::Source
        , virtual public File {
    private:
        FileReader(int fd, const std::string& name, std::size_t pufferSize, int openErrno);

    public:
        static FileReader* open(const std::string& path);

        bool isOpen() override;

        void start() final;
        void suspend() final;
        void resume() final;
        void stop() final;

    private:
        void onEvent(const utils::Timeval& currentTime) override;

        std::size_t pufferSize = 0;

        bool suspended = false;

    protected:
        int openErrno = 0;
        bool running = false;

    private:
        static std::map<std::string path, char
    };

*/

    namespace fs = std::filesystem;

    class FileReader
        : public core::EventReceiver
        , public core::pipe::Source {
    public:
        // Constructs a FileReader for the given file.
        // - filename: the file to read.
        // - chunkSize: the maximum number of bytes to read/send per onEvent() call.
        // - sendCallback: a callback invoked with each chunk.
        FileReader(const std::string& filename,
                   size_t chunkSize,
                   int openErrno,
                   std::string* cachedContent,
                   std::unique_ptr<std::ifstream>& fileStream);

        // Returns true if the file has been completely sent.
        bool isFinished() const;

        // Destructor releases any cache reference held.
        ~FileReader() override;

        static FileReader* open(const std::string& path);
        bool isOpen() override;

        void start() final;
        void suspend() final;
        void resume() final;
        void stop() final;

    private:
        void onEvent(const utils::Timeval& currentTime) override;

        // Top-level private members of FileReader.
        std::string filename;
        std::size_t chunkSize;
        std::size_t offset; // Used when sending from cached content.
        // If caching is enabled, this points to the mutable cache entry content.
        std::string* cachedContent = nullptr;
        // For incremental reading when no valid cached content exists.
        std::unique_ptr<std::ifstream> fileStream;

        bool running = false;
        bool suspended = false;

        // ---------------------------------------------------------------------
        // Private inner class: CacheManager
        // ---------------------------------------------------------------------
        // Manages cached file contents (keyed by filename) with a total size limit
        // and FIFO eviction. Eviction (for a new file) is performed only if the
        // total cache size plus the new file’s size would exceed the limit, and only
        // non‑in‑use entries (i.e. with zero reference count) are removed.
        class CacheManager {
        public:
            explicit CacheManager(size_t maxSize);

            // Checks for an up-to-date cache entry. If found, increments its refCount
            // and returns a pointer to its content. Otherwise, returns nullptr.
            const std::string* getCachedContent(const std::string& filename, fs::file_time_type& cachedTime);

            // Before creating a new cache entry, try to evict non‑in‑use entries
            // so that currentCacheSize + fileSize <= maxCacheSize.
            void evictForFile(size_t fileSize);

            // Creates an empty cache entry for the file.
            void createCacheEntry(const std::string& filename);

            // Returns a mutable pointer to the cache entry’s content string.
            std::string* getMutableCacheEntry(const std::string& filename);

            // Appends appendedSize to the current cache size for the given file.
            // (No eviction is done here.)
            void appendToCacheEntry(size_t appendedSize);

            // Releases (decrementfileStreams the refCount for) the cache entry for the file.
            void releaseCache(const std::string& filename);

            size_t getMaxCacheSize() const;

        private:
            // -----------------------------------------------------------------
            // Private inner class: CachedFile
            // -----------------------------------------------------------------
            // Represents one cached file entry.
            class CachedFile {
            public: // Number of FileReader instances using this entry.
                CachedFile()
                    : refCount(0) {
                }

                std::string content;
                std::list<std::string>::iterator orderIt;
                fs::file_time_type lastWriteTime;
                int refCount;
            };

            size_t maxCacheSize;
            size_t currentCacheSize;
            std::map<std::string, CachedFile> cache;
            std::list<std::string> insertionOrder;
        };

        // A single shared static CacheManager used by all FileReader instances.
        static CacheManager cacheManager;
    };

    // A simple callback that "sends" a chunk by printing it to the console.
    void sendChunk(const char* data, size_t length);

} // namespace core::file

#endif // CORE_FILE_FILEREADER_H
