/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"
#include "core/SNodeC.h"
#include "core/pipe/Source.h"
#include "core/socket/stream/SocketWriter.h"
#include "log/SemanticLogger.h"
#include "net/config/ConfigConnection.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/TestResult.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <functional>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {

    class WriterPolicyConfig final
        : public net::config::ConfigInstance
        , public net::config::ConfigConnection {
    public:
        WriterPolicyConfig()
            : ConfigInstance("writer-policy", Role::CLIENT)
            , ConfigConnection(this) {
        }

        ~WriterPolicyConfig() override = default;

        using ConfigConnection::getMaximumWriteQueueBytes;
        using ConfigConnection::getWriteQueueHighWatermark;
        using ConfigConnection::getWriteQueueLowWatermark;
        using ConfigConnection::setMaximumWriteQueueBytes;
        using ConfigConnection::setWriteQueueHighWatermark;
        using ConfigConnection::setWriteQueueLowWatermark;

        void parseConnection(int argc, char* argv[]) const {
            ConfigConnection::parse(argc, argv);
        }

        bool hasOption(const std::string& name) const {
            return ConfigConnection::getOption(name) != nullptr;
        }

        std::string generatedConfig() const {
            return ConfigInstance::configToStr();
        }
    };

    class CountingSource final : public core::pipe::Source {
    public:
        bool isOpen() override {
            return open;
        }

        void start() override {
            ++starts;
        }

        void suspend() override {
            ++suspends;
        }

        void resume() override {
            ++resumes;
        }

        void stop() override {
            open = false;
            ++stops;
        }

        bool open = true;
        int starts = 0;
        int suspends = 0;
        int resumes = 0;
        int stops = 0;
    };

    logger::LogScope writerLogScope() {
        return {logger::LogOrigin::Framework,
                logger::LogBoundary::Connection,
                "core.socket.stream",
                "SocketWriterResourcePolicyTest",
                logger::LogRole::Client,
                "1"};
    }

    class TestWriter final : public core::socket::stream::SocketWriter {
    public:
        TestWriter(std::size_t blockSize, std::size_t maximum, std::size_t high, std::size_t low)
            : SocketWriter(
                  "SocketWriterResourcePolicyTest",
                  writerLogScope(),
                  [this](int errnum) {
                      lastError = errnum;
                      closeWriter();
                  },
                  {1, 0},
                  blockSize,
                  {1, 0},
                  maximum,
                  high,
                  low) {
        }

        explicit TestWriter(std::size_t blockSize)
            : SocketWriter(
                  "SocketWriterResourcePolicyTest",
                  writerLogScope(),
                  [this](int errnum) {
                      lastError = errnum;
                      closeWriter();
                  },
                  {1, 0},
                  blockSize,
                  {1, 0},
                  0,
                  0,
                  0) {
        }

        bool open(int fd) {
            const bool enabled = enable(fd);
            if (enabled) {
                suspend();
            }
            return enabled;
        }

        void closeWriter() {
            if (isEnabled()) {
                disable();
            }
        }

        bool enqueue(const std::string& bytes) {
            return sendToPeer(bytes.data(), bytes.size());
        }

        void legacyEnqueue(const std::string& bytes) {
            sendToPeer(bytes.data(), bytes.size());
        }

        bool attach(CountingSource* source) {
            return streamToPeer(source);
        }

        void detachSource() {
            streamEof();
        }

        void beginShutdown() {
            shutdownWrite([this]() {
                shutdownCallback = true;
            });
        }

        std::size_t totalQueued() const {
            return getTotalQueued();
        }

        std::size_t queuedBytes() const {
            return writePuffer.size();
        }

        int lastError = 0;
        int writeError = 0;
        int drains = 0;
        std::function<void()> drained;
        bool shutdownCallback = false;
        std::string written;

    private:
        ssize_t write(const char* chunk, std::size_t chunkLen) override {
            ssize_t result = -1;
            if (writeError == 0) {
                written.append(chunk, chunkLen);
                result = static_cast<ssize_t>(chunkLen);
            } else {
                errno = writeError;
            }
            return result;
        }

        void onDrain() override {
            ++drains;
            if (drained) {
                drained();
            }
        }

        bool onSignal(int) override {
            return true;
        }

        void doWriteShutdown(const std::function<void()>& onShutdown) override {
            onShutdown();
        }

        void unobservedEvent() override {
        }
    };

    class SocketPair {
    public:
        SocketPair() {
            if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds) != 0) {
                fds[0] = -1;
                fds[1] = -1;
            }
        }

        ~SocketPair() {
            if (fds[0] >= 0) {
                ::close(fds[0]);
            }
            if (fds[1] >= 0) {
                ::close(fds[1]);
            }
        }

        int writerFd() const {
            return fds[0];
        }

        bool valid() const {
            return fds[0] >= 0 && fds[1] >= 0;
        }

    private:
        int fds[2]{-1, -1};
    };

    void processOneEventLoopTick() {
        sigset_t sigmask;
        sigemptyset(&sigmask);
        static_cast<void>(core::EventLoop::instance().getEventMultiplexer().tick({}, sigmask));
    }

    void releaseDisabledWriters() {
        processOneEventLoopTick();
    }

} // namespace

int main() {
    tests::support::TestResult result;

    char arg0[] = "SocketWriterResourcePolicyTest";
    char* args[] = {arg0, nullptr};
    core::SNodeC::init(1, args);

    {
        WriterPolicyConfig config;
        result.expectTrue(config.hasOption("--maximum-write-queue-bytes"), "maximum queue CLI option is registered");
        result.expectTrue(config.hasOption("--write-queue-high-watermark"), "high-watermark CLI option is registered");
        result.expectTrue(config.hasOption("--write-queue-low-watermark"), "low-watermark CLI option is registered");
        result.expectTrue(config.getMaximumWriteQueueBytes() == 0, "default maximum queue is unlimited");
        result.expectTrue(config.getWriteQueueHighWatermark() == 0, "default high watermark selects legacy automatic behavior");
        result.expectTrue(config.getWriteQueueLowWatermark() == 0, "default low watermark resumes at an empty queue");

        config.setMaximumWriteQueueBytes(4096)->setWriteQueueHighWatermark(3072)->setWriteQueueLowWatermark(1024);
        result.expectTrue(config.getMaximumWriteQueueBytes() == 4096, "C++ maximum queue accessor reflects configuration");
        result.expectTrue(config.getWriteQueueHighWatermark() == 3072, "C++ high-watermark accessor reflects configuration");
        result.expectTrue(config.getWriteQueueLowWatermark() == 1024, "C++ low-watermark accessor reflects configuration");

        char command[] = "connection";
        char highOption[] = "--write-queue-high-watermark";
        char highValue[] = "2048";
        char lowOption[] = "--write-queue-low-watermark";
        char lowValue[] = "512";
        char* connectionArgs[] = {command, highOption, highValue, lowOption, lowValue, nullptr};
        config.parseConnection(5, connectionArgs);
        result.expectTrue(config.getWriteQueueHighWatermark() == 2048, "connection CLI sets the high watermark");
        result.expectTrue(config.getWriteQueueLowWatermark() == 512, "connection CLI sets the low watermark");

        const std::string generatedConfig = config.generatedConfig();
        result.expectTrue(generatedConfig.find("connection.maximum-write-queue-bytes") != std::string::npos,
                          "maximum queue appears in generated configuration-file output");
        result.expectTrue(generatedConfig.find("connection.write-queue-high-watermark") != std::string::npos,
                          "high watermark appears in generated configuration-file output");
        result.expectTrue(generatedConfig.find("connection.write-queue-low-watermark") != std::string::npos,
                          "low watermark appears in generated configuration-file output");
    }

    {
        SocketPair pair;
        result.expectTrue(pair.valid(), "socketpair is available");

        TestWriter writer(4, 10, 6, 2);
        result.expectTrue(!writer.enqueue("x"), "disabled writer reports false");
        result.expectTrue(writer.queuedBytes() == 0, "disabled writer does not queue bytes");
        result.expectTrue(writer.open(pair.writerFd()), "writer descriptor is enabled");

        CountingSource source;
        result.expectTrue(writer.attach(&source), "pipe source attaches");

        result.expectTrue(writer.enqueue("12345"), "bytes below high watermark are queued and report true");
        result.expectEqual(0, source.suspends, "source remains active below high watermark");
        result.expectTrue(!writer.enqueue("6"), "high-watermark boundary is queued and reports false");
        result.expectEqual(1, source.suspends, "source suspends at high watermark");

        result.expectTrue(!writer.enqueue("12345"), "bytes beyond the configured maximum are queued and report false");
        result.expectTrue(writer.totalQueued() == 11, "all accepted bytes contribute to the queued counter");
        result.expectTrue(writer.queuedBytes() == 11, "the entire chunk is queued beyond the watermark");
        result.expectEqual(0, writer.lastError, "queue pressure does not report a write error");
        result.expectEqual(1, source.suspends, "further sends do not repeatedly suspend the source");

        processOneEventLoopTick();
        result.expectTrue(writer.queuedBytes() == 7, "one write drains one configured block");
        result.expectEqual(0, source.resumes, "source stays suspended above the low watermark");
        result.expectEqual(0, writer.drains, "no drain notification above the low watermark");
        processOneEventLoopTick();
        result.expectTrue(writer.queuedBytes() == 3, "a second write continues draining");
        result.expectEqual(0, source.resumes, "source stays suspended just above the low watermark");
        result.expectEqual(0, writer.drains, "no drain notification just above the low watermark");
        processOneEventLoopTick();
        result.expectTrue(writer.queuedBytes() == 0, "the complete queue drains");
        result.expectEqual(1, source.resumes, "source resumes below the low watermark");
        result.expectEqual(1, writer.drains, "one drain notification below the low watermark despite repeated pressured sends");
        result.expectTrue(writer.written == "12345612345", "all bytes are written once and in order after false returns");
        result.expectTrue(!writer.enqueue("123456"), "a new chunk at the watermark queues and reports false");
        processOneEventLoopTick();
        result.expectTrue(writer.queuedBytes() == 2, "one block drains the queue to the exact low watermark");
        result.expectEqual(2, source.resumes, "source resumes at the exact low watermark");
        result.expectEqual(2, writer.drains, "a second pressure cycle notifies at the exact low watermark");
        result.expectTrue(writer.enqueue("a"), "sending reports true again below the high watermark");
        result.expectTrue(!writer.enqueue("1234567"), "queue may reach the configured maximum without rejection");
        result.expectTrue(writer.queuedBytes() == 10, "all bytes at the maximum are retained");
        result.expectTrue(!writer.enqueue("x"), "sending above the maximum still queues bytes and reports false");
        result.expectTrue(writer.queuedBytes() == 11, "the queue may exceed the maximum watermark bound");

        writer.detachSource();
        writer.closeWriter();
        result.expectTrue(!writer.enqueue("x"), "closed writer reports false");
        result.expectTrue(writer.queuedBytes() == 11, "closed writer does not append bytes");
        releaseDisabledWriters();
    }

    {
        SocketPair pair;
        TestWriter writer(4, 4, 4, 0);
        result.expectTrue(writer.open(pair.writerFd()), "writer with an ignored send result is enabled");
        writer.legacyEnqueue("12345");
        result.expectEqual(0, writer.lastError, "ignoring the send result does not turn queue pressure into a connection error");
        result.expectTrue(writer.queuedBytes() == 5, "ignoring the send result still queues the complete chunk");
        while (writer.queuedBytes() != 0) {
            processOneEventLoopTick();
        }
        result.expectTrue(writer.written == "12345", "an older caller still delivers all bytes");
        writer.closeWriter();
        releaseDisabledWriters();
    }

    {
        SocketPair pairA;
        SocketPair pairB;
        TestWriter writerA(4, 4, 4, 0);
        TestWriter writerB(4, 8, 8, 0);
        result.expectTrue(writerA.open(pairA.writerFd()) && writerB.open(pairB.writerFd()), "two writers are enabled");
        result.expectTrue(!writerA.enqueue("1234"), "first connection reports pressure at its own watermark");
        result.expectTrue(!writerA.enqueue("5"), "first connection keeps queuing above its watermark");
        result.expectTrue(writerB.enqueue("12345"), "second connection has an independent larger watermark");
        result.expectTrue(writerA.queuedBytes() == 5, "first connection retains its entire payload");
        result.expectTrue(writerB.queuedBytes() == 5, "second connection retains its independent payload");
        writerA.closeWriter();
        writerB.closeWriter();
        releaseDisabledWriters();
    }

    {
        SocketPair pair;
        TestWriter writer(4);
        CountingSource source;
        result.expectTrue(writer.open(pair.writerFd()) && writer.attach(&source), "default-policy writer and source are ready");
        result.expectTrue(writer.enqueue(std::string(20, 'a')), "default threshold minus one reports true");
        result.expectEqual(0, source.suspends, "legacy source remains active at five blocks");
        result.expectTrue(!writer.enqueue("b"), "default threshold reports false without rejecting bytes");
        result.expectTrue(writer.queuedBytes() == 21, "default threshold retains the entire payload");
        result.expectEqual(1, source.suspends, "legacy source suspends above five blocks");
        while (writer.queuedBytes() != 0) {
            processOneEventLoopTick();
        }
        result.expectEqual(1, source.resumes, "legacy default resumes only when queue is empty");
        writer.detachSource();
        writer.closeWriter();
        releaseDisabledWriters();
    }

    {
        SocketPair pair;
        TestWriter writer(4, 10, 6, 2);
        result.expectTrue(writer.open(pair.writerFd()), "shutdown writer is enabled");
        writer.beginShutdown();
        result.expectTrue(writer.shutdownCallback, "empty queue starts shutdown immediately");
        result.expectTrue(!writer.enqueue("x"), "shutdown writer reports false");
        result.expectTrue(writer.queuedBytes() == 0, "shutdown writer does not append bytes");
        writer.closeWriter();
        releaseDisabledWriters();
    }

    {
        SocketPair pair;
        TestWriter writer(4, 6, 6, 0);
        CountingSource source;
        result.expectTrue(writer.open(pair.writerFd()) && writer.attach(&source), "shutdown backpressure writer and source are ready");
        result.expectTrue(!writer.enqueue("123456"), "shutdown fixture queues bytes and reports pressure at its high watermark");
        result.expectEqual(1, source.suspends, "shutdown fixture suspends its source");
        writer.beginShutdown();
        while (writer.queuedBytes() != 0) {
            processOneEventLoopTick();
        }
        result.expectEqual(0, source.resumes, "draining for shutdown does not resume a suspended source");
        result.expectEqual(0, writer.drains, "write shutdown suppresses drain notification");
        writer.detachSource();
        writer.closeWriter();
        releaseDisabledWriters();
    }

    {
        SocketPair pair;
        TestWriter writer(4, 0, 6, 2);
        result.expectTrue(writer.open(pair.writerFd()), "unpressured writer is enabled");
        result.expectTrue(writer.enqueue("1234"), "ordinary write stays below the high watermark");
        processOneEventLoopTick();
        processOneEventLoopTick();
        result.expectEqual(0, writer.drains, "ordinary writes and empty events do not notify");
        writer.closeWriter();
        releaseDisabledWriters();
    }

    {
        SocketPair pair;
        TestWriter writer(4, 0, 6, 2);
        result.expectTrue(writer.open(pair.writerFd()), "reentrant writer is enabled");
        writer.drained = [&]() {
            if (writer.drains == 1) {
                result.expectTrue(!writer.enqueue("abcd"), "a drain callback may establish another pressure cycle");
            }
        };
        result.expectTrue(!writer.enqueue("123456"), "reentrant fixture reaches the high watermark");
        processOneEventLoopTick();
        result.expectEqual(1, writer.drains, "first cycle notifies once");
        result.expectTrue(writer.queuedBytes() == 6, "callback queues subsequent data without recursion");
        processOneEventLoopTick();
        result.expectEqual(2, writer.drains, "reentrant cycle retains its own notification");
        processOneEventLoopTick();
        processOneEventLoopTick();
        result.expectEqual(2, writer.drains, "further draining does not duplicate notifications");
        result.expectTrue(writer.written == "123456abcd", "reentrant sends preserve byte order");
        writer.closeWriter();
        releaseDisabledWriters();
    }

    {
        SocketPair pair;
        TestWriter writer(4, 0, 6, 2);
        result.expectTrue(writer.open(pair.writerFd()), "closing callback writer is enabled");
        writer.drained = [&]() {
            writer.closeWriter();
        };
        result.expectTrue(!writer.enqueue("123456"), "closing callback fixture reaches high watermark");
        processOneEventLoopTick();
        result.expectEqual(1, writer.drains, "a drain callback may close the writer");
        result.expectTrue(!writer.enqueue("x"), "callback closure leaves writer disabled");
        releaseDisabledWriters();
        result.expectEqual(1, writer.drains, "closure does not cause another notification");
    }

    {
        SocketPair pair;
        TestWriter writer(4, 0, 6, 2);
        result.expectTrue(writer.open(pair.writerFd()), "failing writer is enabled");
        result.expectTrue(!writer.enqueue("123456"), "failing fixture reaches high watermark");
        writer.writeError = EIO;
        processOneEventLoopTick();
        result.expectEqual(EIO, writer.lastError, "write failure follows the existing error path");
        releaseDisabledWriters();
        result.expectEqual(0, writer.drains, "write failure does not notify drain");
    }

    core::SNodeC::free();

    return result.processResult();
}
