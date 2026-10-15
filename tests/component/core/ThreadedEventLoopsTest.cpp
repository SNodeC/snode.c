// SPDX-License-Identifier: MIT OR LGPL-3.0-or-later
#include "core/EventLoop.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/Shutdown.h"
#include "core/pipe/Pipe.h"
#include "core/pipe/PipeSink.h"
#include "core/timer/Timer.h"
#include "log/Logger.h"
#include "support/TestResult.h"
#include "utils/Config.h"
#include "utils/SubCommand.h"

#include <array>
#include <atomic>
#include <barrier>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>

namespace {
    bool cleanupCalled = false;

    class CleanupProbe final : public utils::SubCommand {
    public:
        explicit CleanupProbe(utils::SubCommand* parent)
            : utils::SubCommand(parent, std::make_shared<utils::AppWithPtr>("Cleanup probe", "cleanup-probe", this), "Tests") {
        }
        ~CleanupProbe() override {
            cleanupCalled = !core::SNodeC::reconfigure();
            core::SNodeC::free();
        }
    };

    struct Observation {
        core::EventLoop* loop = nullptr;
        core::EventMultiplexer* mux = nullptr;
        int result = -99, next = 0, timers = 0, reads = 0, shutdowns = 0;
        unsigned long initialTicks = 99, finalTicks = 0;
        bool local = true, initialized = false, stopping = false, reconfigureRejected = false;
        core::ShutdownContext context;
    };

    std::size_t descriptorCount() {
        const std::filesystem::path path("/proc/self/fd");
        return std::filesystem::exists(path) ? std::distance(std::filesystem::directory_iterator(path), {}) : 0;
    }
} // namespace

int main(int argc, char** argv) {
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("ThreadedEventLoopsTest");
        return tests::support::cTestSkipReturnCode;
    }
    tests::support::TestResult result;
    const std::string scenario = argc > 1 ? argv[1] : "isolation";
    char quiet[] = "--quiet", help[] = "--help";
    char* arguments[] = {argv[0], quiet, scenario == "help" ? help : nullptr, nullptr};
    const int argumentCount = scenario == "help" ? 3 : 2;
    core::SNodeC::init(argumentCount, arguments);
    utils::Config::configRoot.newSubCommand<CleanupProbe>();
    const auto fdsBefore = descriptorCount();
    struct sigaction previous{};
    ::sigaction(SIGTERM, nullptr, &previous);

    if (scenario == "churn") {
        for (int i = 0; i < 12; ++i) {
            Observation observed;
            std::thread worker([&] {
                core::SNodeC::init(argumentCount, arguments);
                observed.initialTicks = core::EventLoop::getTickCounter();
                auto timer = core::timer::Timer::singleshotTimer(
                    [] {
                        core::SNodeC::stop();
                    },
                    utils::Timeval(0.001));
                observed.result = core::SNodeC::start();
                core::SNodeC::free();
            });
            worker.join();
            result.expectEqual(0, observed.result, "each new thread runs and stops its loop");
            result.expectTrue(observed.initialTicks == 0, "each new thread starts with its own tick counter");
        }
        result.expectTrue(descriptorCount() == fdsBefore, "exiting worker threads release backend descriptors");
        core::SNodeC::free();
        result.expectTrue(cleanupCalled, "configuration cleanup can reenter lifecycle queries");
        return result.processResult();
    }

    char logPath[] = "/tmp/snodec-threaded-loops-XXXXXX";
    const int logFd = ::mkstemp(logPath);
    result.expectTrue(logFd >= 0, "create a log file for concurrent producers");
    if (logFd >= 0) {
        ::close(logFd);
        logger::Logger::logToFile(logPath);
    }
    std::array<Observation, 2> observed;
    std::array<std::thread, 2> workers;
    std::barrier phase(3);
    std::atomic<bool> independent{false}, firstFinished{false};
    std::atomic<int> ready{0};
    for (int i = 0; i < 2; ++i) {
        workers[i] = std::thread([&, i] {
            auto& own = observed[i];
            const auto owner = std::this_thread::get_id();
            own.loop = &core::EventLoop::instance();
            own.mux = &own.loop->getEventMultiplexer();
            phase.arrive_and_wait();
            phase.arrive_and_wait();
            // Reject the old singleton implementation without concurrently driving it.
            if (!independent.load()) {
                return;
            }
            core::SNodeC::init(argumentCount, arguments);
            own.initialized = core::SNodeC::state() == core::State::INITIALIZED;
            own.initialTicks = core::EventLoop::getTickCounter();
            auto checkThread = [&] {
                own.local = own.local && std::this_thread::get_id() == owner;
            };
            core::pipe::Pipe pipe;
            auto* sink = pipe.releaseReadAsSink(1024, core::DescriptorEventReceiver::TIMEOUT::DISABLE);
            if (sink) {
                sink->setOnData([&](const char*, std::size_t size) {
                    checkThread();
                    own.reads += static_cast<int>(size);
                });
                sink->setOnShutdown([&](const core::ShutdownContext& context) {
                    checkThread();
                    ++own.shutdowns;
                    own.context = context;
                    sink->close();
                });
            }
            if (scenario != "signal") {
                const char byte = 'x';
                static_cast<void>(::write(pipe.getWriteFd(), &byte, 1));
            }
            core::EventReceiver::atNextTick([&] {
                checkThread();
                ++own.next;
                ready.fetch_add(1);
                own.reconfigureRejected = i != 0 || !core::SNodeC::reconfigure();
            });
            core::timer::Timer timer;
            if (scenario != "signal" && scenario != "help") {
                timer = core::timer::Timer::intervalTimer(
                    [&] {
                        checkThread();
                        ++own.timers;
                        if (i == 0 || scenario == "tick" || firstFinished.load()) {
                            core::EventReceiver::atNextTick([&] {
                                checkThread();
                                ++own.next;
                                core::SNodeC::stop();
                            });
                            timer.cancel();
                        }
                    },
                    utils::Timeval(0.005));
            }
            phase.arrive_and_wait();
            phase.arrive_and_wait();
            if (scenario == "late-start" && i == 1) {
                while (!firstFinished.load()) {
                    std::this_thread::yield();
                }
            }
            if (scenario == "tick") {
                do {
                    const auto status = core::SNodeC::tick(utils::Timeval(0.02));
                    if (status == core::TickStatus::TRACE || status == core::TickStatus::NOOBSERVER) {
                        break;
                    }
                } while (core::SNodeC::state() != core::State::STOPPING);
                own.result = 0;
                core::SNodeC::free();
            } else {
                own.result = core::SNodeC::start();
            }
            own.stopping = core::SNodeC::state() == core::State::STOPPING;
            own.finalTicks = core::EventLoop::getTickCounter();
            own.loop->log().info("threaded-loop-finished-{}", i);
            core::SNodeC::free(); // Existing applications commonly free again after start.
            if (i == 0) {
                firstFinished.store(true);
            }
        });
    }
    phase.arrive_and_wait();
    independent.store(observed[0].loop != observed[1].loop && observed[0].mux != observed[1].mux);
    result.expectTrue(independent.load(), "each thread owns an independent loop and backend");
    phase.arrive_and_wait();
    if (independent.load()) {
        phase.arrive_and_wait();
        if (scenario == "isolation" || scenario == "late-start") {
            result.expectEqual(0, core::SNodeC::start(), "the initial thread can finish before the workers start");
        }
        phase.arrive_and_wait();
        if (scenario == "signal") {
            while (ready.load() != 2) {
                std::this_thread::yield();
            }
            // Let the workers enter their indefinite descriptor waits first.
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            // Deliver to the control thread, interrupting neither worker's descriptor wait.
            ::raise(SIGTERM);
        }
    }
    for (auto& worker : workers) {
        worker.join();
    }
    core::SNodeC::free();
    logger::Logger::disableLogToFile();
    result.expectTrue(cleanupCalled, "configuration cleanup can reenter lifecycle queries");
    if (independent.load()) {
        for (const auto& own : observed) {
            result.expectTrue(own.initialized && own.initialTicks == 0, "loop state and tick counter are thread local");
            result.expectTrue(own.local && own.stopping, "callbacks and shutdown remain on their owning thread");
            result.expectEqual(scenario == "signal" ? -SIGTERM : scenario == "help" ? 2 : 0, own.result, "loop exit status");
            result.expectEqual(1, own.shutdowns, "each descriptor receives exactly one shutdown notification");
            if (scenario == "signal") {
                result.expectTrue(own.context.reason == core::ShutdownReason::Signal && own.context.signal == SIGTERM,
                                  "every loop observes a signal delivered to a different thread");
            } else if (scenario != "help") {
                result.expectEqual(2, own.next, "deferred callbacks run before and after timer dispatch");
                result.expectEqual(1, own.reads, "each loop dispatches its own descriptor");
                result.expectTrue(own.timers > 0 && own.finalTicks > 0, "each loop dispatches timers and advances ticks");
                result.expectTrue(own.context.reason == core::ShutdownReason::Requested, "local stop retains requested shutdown reason");
                result.expectTrue(own.reconfigureRejected, "shared configuration cannot be reparsed with multiple attached loops");
            }
        }
        result.expectTrue(descriptorCount() == fdsBefore, "worker backend descriptors are closed on thread exit");
    }
    struct sigaction restored{};
    ::sigaction(SIGTERM, nullptr, &restored);
    result.expectTrue(restored.sa_handler == previous.sa_handler, "the last loop restores process signal disposition");
    if (logFd >= 0) {
        std::ifstream log(logPath);
        const std::string records{std::istreambuf_iterator<char>(log), {}};
        if (independent.load() && scenario != "help") {
            result.expectTrue(records.find("threaded-loop-finished-0") != std::string::npos &&
                                  records.find("threaded-loop-finished-1") != std::string::npos,
                              "shared logging survives loop exit and drains both producers");
        }
        ::unlink(logPath);
    }
    return result.processResult();
}
