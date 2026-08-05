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
#include "core/socket/stream/QueueResult.h"
#include "core/socket/stream/SocketWriter.h"
#include "net/config/ConfigConnection.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/TestResult.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {

    using core::socket::stream::QueueResult;

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

    class TestWriter final : public core::socket::stream::SocketWriter {
    public:
        TestWriter(std::size_t blockSize, std::size_t maximum, std::size_t high, std::size_t low)
            : SocketWriter(
                  "SocketWriterResourcePolicyTest",
                  [this](int errnum) {
                      lastError = errnum;
                  },
                  {1, 0},
                  blockSize,
                  {1, 0},
                  maximum,
                  high,
                  low) {
        }

        explicit TestWriter(std::size_t blockSize)
            : SocketWriter("SocketWriterResourcePolicyTest",
                           [this](int errnum) {
                               lastError = errnum;
                           },
                           {1, 0},
                           blockSize,
                           {1, 0}) {
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

        QueueResult enqueue(const std::string& bytes) {
            return trySendToPeer(bytes.data(), bytes.size());
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
        bool shutdownCallback = false;

    private:
        ssize_t write(const char*, std::size_t chunkLen) override {
            return static_cast<ssize_t>(chunkLen);
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

    void expectResult(tests::support::TestResult& result, QueueResult expected, QueueResult actual, const std::string& message) {
        result.expectEqual(static_cast<int>(expected), static_cast<int>(actual), message);
    }

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
        expectResult(result, QueueResult::Closed, writer.enqueue("x"), "disabled writer reports Closed");
        result.expectTrue(writer.open(pair.writerFd()), "writer descriptor is enabled");

        CountingSource source;
        result.expectTrue(writer.attach(&source), "pipe source attaches");

        expectResult(result, QueueResult::Queued, writer.enqueue("12345"), "bytes below high watermark are queued");
        result.expectEqual(0, source.suspends, "source remains active below high watermark");
        expectResult(result, QueueResult::Queued, writer.enqueue("6"), "high-watermark boundary is admitted");
        result.expectEqual(1, source.suspends, "source suspends at high watermark");

        const std::size_t totalBeforeRejection = writer.totalQueued();
        expectResult(result, QueueResult::WouldExceedLimit, writer.enqueue("12345"), "chunk that would exceed maximum queue is rejected");
        result.expectTrue(writer.totalQueued() == totalBeforeRejection, "rejection does not increment total queued bytes");
        result.expectTrue(writer.queuedBytes() == 6, "rejection leaves connection-local queue unchanged");

        processOneEventLoopTick();
        result.expectTrue(writer.queuedBytes() == 2, "one write drains one configured block");
        result.expectEqual(1, source.resumes, "source resumes at low watermark");

        expectResult(result, QueueResult::Queued, writer.enqueue("12345678"), "exact queue limit is admitted");
        result.expectTrue(writer.queuedBytes() == 10, "queue reaches exact configured maximum");
        expectResult(result, QueueResult::WouldExceedLimit, writer.enqueue("x"), "byte beyond exact limit is rejected");

        writer.detachSource();
        writer.closeWriter();
        expectResult(result, QueueResult::Closed, writer.enqueue("x"), "closed writer reports Closed");
        releaseDisabledWriters();
    }

    {
        SocketPair pair;
        TestWriter writer(4, 4, 4, 0);
        result.expectTrue(writer.open(pair.writerFd()), "legacy-send overflow writer is enabled");
        writer.legacyEnqueue("12345");
        result.expectEqual(ENOBUFS, writer.lastError, "legacy void send reports finite-queue overflow as a connection error");
        result.expectEqual(std::size_t{0}, writer.queuedBytes(), "legacy overflow does not append a partial chunk");
        writer.closeWriter();
        releaseDisabledWriters();
    }

    {
        SocketPair pairA;
        SocketPair pairB;
        TestWriter writerA(4, 4, 4, 0);
        TestWriter writerB(4, 8, 8, 0);
        result.expectTrue(writerA.open(pairA.writerFd()) && writerB.open(pairB.writerFd()), "two writers are enabled");
        expectResult(result, QueueResult::Queued, writerA.enqueue("1234"), "first connection admits its own maximum");
        expectResult(result, QueueResult::WouldExceedLimit, writerA.enqueue("5"), "first connection enforces its limit");
        expectResult(result, QueueResult::Queued, writerB.enqueue("12345"), "second connection has an independent larger limit");
        writerA.closeWriter();
        writerB.closeWriter();
        releaseDisabledWriters();
    }

    {
        SocketPair pair;
        TestWriter writer(4);
        CountingSource source;
        result.expectTrue(writer.open(pair.writerFd()) && writer.attach(&source), "default-policy writer and source are ready");
        expectResult(result, QueueResult::Queued, writer.enqueue(std::string(20, 'a')), "legacy threshold minus one remains admitted");
        result.expectEqual(0, source.suspends, "legacy source remains active at five blocks");
        expectResult(result, QueueResult::Queued, writer.enqueue("b"), "default queue remains unlimited");
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
        expectResult(result, QueueResult::ShutdownInProgress, writer.enqueue("x"), "shutdown writer reports ShutdownInProgress");
        writer.closeWriter();
        releaseDisabledWriters();
    }

    {
        SocketPair pair;
        TestWriter writer(4, 6, 6, 0);
        CountingSource source;
        result.expectTrue(writer.open(pair.writerFd()) && writer.attach(&source), "shutdown backpressure writer and source are ready");
        expectResult(result, QueueResult::Queued, writer.enqueue("123456"), "shutdown fixture reaches its high watermark");
        result.expectEqual(1, source.suspends, "shutdown fixture suspends its source");
        writer.beginShutdown();
        while (writer.queuedBytes() != 0) {
            processOneEventLoopTick();
        }
        result.expectEqual(0, source.resumes, "draining for shutdown does not resume a suspended source");
        writer.detachSource();
        writer.closeWriter();
        releaseDisabledWriters();
    }

    core::SNodeC::free();

    return result.processResult();
}
