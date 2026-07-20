/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/stdio/StdioTransport.h"

#include "ai/openai/codex/AppServerClient.h"
#include "core/SNodeC.h"
#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/pipe/Pipe.h"
#include "core/pipe/PipeSink.h"
#include "core/pipe/PipeSource.h"
#include "core/system/unistd.h"
#include "core/timer/Timer.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"

#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <spawn.h>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <sys/wait.h>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <sys/syscall.h>
#endif

extern char** environ;

namespace ai::openai::codex::stdio::detail {

    namespace {
        constexpr std::size_t MAX_FRAMED_LINE_BYTES = 1024 * 1024;
        const utils::Timeval CHILD_EXIT_POLL_INTERVAL = {0, 20000};
        const utils::Timeval STDIN_FLUSH_INTERVAL = {0, 250000};
        const utils::Timeval SIGTERM_GRACE_INTERVAL = {0, 250000};
        const utils::Timeval SIGKILL_GRACE_INTERVAL = {0, 250000};

        bool setNonBlocking(int fd, int& errnum) {
            int flags = -1;
            do {
                flags = ::fcntl(fd, F_GETFL);
            } while (flags < 0 && errno == EINTR);

            int result = -1;
            if (flags >= 0) {
                do {
                    result = ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
                } while (result < 0 && errno == EINTR);
            }

            if (flags < 0 || result != 0) {
                errnum = errno;
                return false;
            }
            return true;
        }

        std::string errorText(const std::string& operation, int errnum) {
            return operation + ": " + std::strerror(errnum);
        }

        class LineFramer {
        public:
            using LineHandler = std::function<bool(std::string)>;

            bool feed(const char* data, std::size_t length, const LineHandler& handler, std::string& errorMessage) {
                buffer.append(data, length);

                std::size_t consumed = 0;
                while (true) {
                    const std::size_t newline = buffer.find('\n', consumed);
                    if (newline == std::string::npos) {
                        break;
                    }
                    if (newline - consumed > MAX_FRAMED_LINE_BYTES) {
                        errorMessage = "framed line exceeds the input limit";
                        return false;
                    }

                    std::string line = buffer.substr(consumed, newline - consumed);
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    consumed = newline + 1;

                    if (!handler(std::move(line))) {
                        if (consumed > 0) {
                            buffer.erase(0, consumed);
                        }
                        return false;
                    }
                }

                if (consumed > 0) {
                    buffer.erase(0, consumed);
                }
                if (buffer.size() > MAX_FRAMED_LINE_BYTES) {
                    errorMessage = "framed line exceeds the input limit";
                    return false;
                }

                return true;
            }

            bool finish(const LineHandler& handler) {
                if (buffer.empty()) {
                    return true;
                }

                std::string line = std::move(buffer);
                buffer.clear();
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                return handler(std::move(line));
            }

        private:
            std::string buffer;
        };

        std::vector<char*> buildArgv(std::string& executable, std::vector<std::string>& arguments) {
            std::vector<char*> argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(executable.data());
            for (std::string& argument : arguments) {
                argv.push_back(argument.data());
            }
            argv.push_back(nullptr);
            return argv;
        }

        int openPidFd(pid_t pid) {
#if defined(__linux__) && defined(SYS_pidfd_open)
            return static_cast<int>(::syscall(SYS_pidfd_open, pid, 0));
#else
            static_cast<void>(pid);
            errno = ENOSYS;
            return -1;
#endif
        }

        int relocateAboveStandardDescriptors(int fd) {
            if (fd < 0 || fd > STDERR_FILENO) {
                return fd;
            }

            const int relocatedFd = ::fcntl(fd, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
            const int savedErrno = errno;
            core::system::close(fd);
            errno = savedErrno;
            return relocatedFd;
        }

        class StandardDescriptorReservations {
        public:
            StandardDescriptorReservations() {
                for (int fd = STDIN_FILENO; fd <= STDERR_FILENO; ++fd) {
                    int descriptorFlags = -1;
                    do {
                        descriptorFlags = ::fcntl(fd, F_GETFD);
                    } while (descriptorFlags < 0 && errno == EINTR);

                    if (descriptorFlags >= 0) {
                        continue;
                    }
                    if (errno != EBADF) {
                        error = errno;
                        return;
                    }

                    int reservation = -1;
                    do {
                        reservation = ::open("/dev/null", O_RDWR | O_CLOEXEC);
                    } while (reservation < 0 && errno == EINTR);
                    if (reservation != fd) {
                        error = reservation < 0 ? errno : EBUSY;
                        if (reservation >= 0) {
                            core::system::close(reservation);
                        }
                        return;
                    }
                    reserved[static_cast<std::size_t>(fd)] = true;
                }
            }

            StandardDescriptorReservations(const StandardDescriptorReservations&) = delete;
            StandardDescriptorReservations& operator=(const StandardDescriptorReservations&) = delete;

            ~StandardDescriptorReservations() {
                for (int fd = STDIN_FILENO; fd <= STDERR_FILENO; ++fd) {
                    if (reserved[static_cast<std::size_t>(fd)]) {
                        core::system::close(fd);
                    }
                }
            }

            bool isValid() const noexcept {
                return error == 0;
            }

            int getError() const noexcept {
                return error;
            }

        private:
            std::array<bool, STDERR_FILENO + 1> reserved{};
            int error = 0;
        };
    } // namespace

    namespace {
        class StdinPipeSource final : public core::pipe::PipeSource {
        public:
            StdinPipeSource(int fd, const std::shared_ptr<StdioTransport::Session>& session);

        private:
            void onShutdown(const core::ShutdownContext& context) override;

            std::weak_ptr<StdioTransport::Session> session;
        };

        class OutputPipeSink final : public core::pipe::PipeSink {
        public:
            OutputPipeSink(int fd, const std::shared_ptr<StdioTransport::Session>& session);

        private:
            void onShutdown(const core::ShutdownContext& context) override;

            std::weak_ptr<StdioTransport::Session> session;
        };

        class LifecycleReceiver final : public core::eventreceiver::ReadEventReceiver {
        public:
            LifecycleReceiver(int readFd, int writeFd, const std::shared_ptr<StdioTransport::Session>& session);
            ~LifecycleReceiver() override;

            void detach();

        private:
            void readEvent() override;
            void readTimeout() override;
            void unobservedEvent() override;
            void onShutdown(const core::ShutdownContext& context) override;

            std::shared_ptr<StdioTransport::Session> session;
            int writeFd;
        };

        class ChildExitReceiver final : public core::eventreceiver::ReadEventReceiver {
        public:
            ChildExitReceiver(int pidfd, const std::shared_ptr<StdioTransport::Session>& session);
            ~ChildExitReceiver() override;

            void detach();

        private:
            void readEvent() override;
            void unobservedEvent() override;
            void onShutdown(const core::ShutdownContext& context) override;

            std::shared_ptr<StdioTransport::Session> session;
        };
    } // namespace

    class StdioTransport::Session : public std::enable_shared_from_this<StdioTransport::Session> {
    public:
        Session(std::string executable,
                std::vector<std::string> arguments,
                bool forceChildExitPollingForTests,
                bool failParentSetupForTests,
                codex::detail::TransportCallbacks callbacks)
            : executable(std::move(executable))
            , arguments(std::move(arguments))
            , forceChildExitPollingForTests(forceChildExitPollingForTests)
            , failParentSetupForTests(failParentSetupForTests)
            , callbacks(std::move(callbacks))
            , logScope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "ai.openai.codex.stdio") {
        }

        ~Session() {
            closeEndpoints();
            if (processObserver != nullptr) {
                processObserver->detach();
                processObserver = nullptr;
            }
            detachLifecycleObserver();
        }

        void setCallbacks(codex::detail::TransportCallbacks callbacks) {
            this->callbacks = std::move(callbacks);
        }

        void start() {
            if (core::SNodeC::state() == core::State::STOPPING) {
                reportError({Error::Category::Launch, ECANCELED, "codex app-server launch rejected during event-loop shutdown"});
                return;
            }

            StandardDescriptorReservations standardDescriptors;
            if (!standardDescriptors.isValid()) {
                const int errnum = standardDescriptors.getError();
                reportError({Error::Category::Launch, errnum, errorText("unable to reserve standard descriptors", errnum)});
                return;
            }

            core::pipe::Pipe lifecyclePipe(O_CLOEXEC | O_NONBLOCK);
            if (!lifecyclePipe.isValid()) {
                const int errnum = lifecyclePipe.getError();
                reportError({Error::Category::Launch, errnum, errorText("unable to create app-server lifecycle pipe", errnum)});
                return;
            }

            try {
                LifecycleReceiver* const receiver =
                    new LifecycleReceiver(lifecyclePipe.getReadFd(), lifecyclePipe.getWriteFd(), shared_from_this());
                static_cast<void>(lifecyclePipe.releaseReadFd());
                static_cast<void>(lifecyclePipe.releaseWriteFd());
                lifecycleObserver = receiver;
            } catch (...) {
                reportError({Error::Category::Launch, ENOMEM, "unable to create app-server lifecycle receiver"});
                return;
            }

            const std::weak_ptr<Session> weakSession = weak_from_this();

            core::pipe::Pipe stdinPipe(O_CLOEXEC);
            core::pipe::Pipe stdoutPipe(O_CLOEXEC);
            core::pipe::Pipe stderrPipe(O_CLOEXEC);

            const auto completePipe = [](const core::pipe::Pipe& pipe) {
                return pipe.isValid();
            };
            if (!completePipe(stdinPipe) || !completePipe(stdoutPipe) || !completePipe(stderrPipe)) {
                const int errnum = !completePipe(stdinPipe)    ? stdinPipe.getError()
                                   : !completePipe(stdoutPipe) ? stdoutPipe.getError()
                                                               : stderrPipe.getError();
                detachLifecycleObserver();
                reportError({Error::Category::Launch, errnum, errorText("unable to create app-server pipes", errnum)});
                return;
            }

            const auto endpointsAreAboveStandardDescriptors = [](const core::pipe::Pipe& pipe) {
                return pipe.getReadFd() > STDERR_FILENO && pipe.getWriteFd() > STDERR_FILENO;
            };
            if (!endpointsAreAboveStandardDescriptors(stdinPipe) || !endpointsAreAboveStandardDescriptors(stdoutPipe) ||
                !endpointsAreAboveStandardDescriptors(stderrPipe)) {
                detachLifecycleObserver();
                reportError({Error::Category::Launch, EINVAL, "app-server pipe endpoint collided with a standard descriptor"});
                return;
            }

            posix_spawn_file_actions_t fileActions{};
            const int actionsInitResult = posix_spawn_file_actions_init(&fileActions);
            if (actionsInitResult != 0) {
                detachLifecycleObserver();
                reportError(
                    {Error::Category::Launch, actionsInitResult, errorText("posix_spawn_file_actions_init failed", actionsInitResult)});
                return;
            }

            int setupResult = 0;
            const auto addAction = [&setupResult](int result) {
                if (setupResult == 0 && result != 0) {
                    setupResult = result;
                }
            };

            addAction(posix_spawn_file_actions_adddup2(&fileActions, stdinPipe.getReadFd(), STDIN_FILENO));
            addAction(posix_spawn_file_actions_adddup2(&fileActions, stdoutPipe.getWriteFd(), STDOUT_FILENO));
            addAction(posix_spawn_file_actions_adddup2(&fileActions, stderrPipe.getWriteFd(), STDERR_FILENO));
            for (const int fd : {stdinPipe.getReadFd(),
                                 stdinPipe.getWriteFd(),
                                 stdoutPipe.getReadFd(),
                                 stdoutPipe.getWriteFd(),
                                 stderrPipe.getReadFd(),
                                 stderrPipe.getWriteFd()}) {
                addAction(posix_spawn_file_actions_addclose(&fileActions, fd));
            }

            posix_spawnattr_t attributes{};
            const int attributesInitResult = posix_spawnattr_init(&attributes);
            if (setupResult == 0 && attributesInitResult != 0) {
                setupResult = attributesInitResult;
            }

            if (attributesInitResult == 0) {
                sigset_t defaultSignals{};
                sigset_t emptyMask{};
                if (::sigemptyset(&defaultSignals) != 0 || ::sigaddset(&defaultSignals, SIGPIPE) != 0 ||
                    ::sigaddset(&defaultSignals, SIGINT) != 0 || ::sigaddset(&defaultSignals, SIGTERM) != 0 ||
                    ::sigaddset(&defaultSignals, SIGALRM) != 0 || ::sigaddset(&defaultSignals, SIGHUP) != 0 ||
                    ::sigemptyset(&emptyMask) != 0) {
                    setupResult = errno;
                }

                if (setupResult == 0) {
                    addAction(posix_spawnattr_setpgroup(&attributes, 0));
                    addAction(posix_spawnattr_setsigdefault(&attributes, &defaultSignals));
                    addAction(posix_spawnattr_setsigmask(&attributes, &emptyMask));

                    constexpr short spawnFlags = static_cast<short>(POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK);
                    addAction(posix_spawnattr_setflags(&attributes, spawnFlags));
                }
            }

            pid_t spawnedPid = -1;
            if (setupResult == 0 && core::SNodeC::state() == core::State::STOPPING) {
                setupResult = ECANCELED;
            }
            if (setupResult == 0) {
                try {
                    std::vector<char*> argv = buildArgv(executable, arguments);
                    setupResult = posix_spawnp(&spawnedPid, executable.c_str(), &fileActions, &attributes, argv.data(), environ);
                } catch (...) {
                    setupResult = ENOMEM;
                }
            }

            if (attributesInitResult == 0) {
                static_cast<void>(posix_spawnattr_destroy(&attributes));
            }
            static_cast<void>(posix_spawn_file_actions_destroy(&fileActions));

            if (setupResult != 0) {
                detachLifecycleObserver();
                reportError({Error::Category::Launch, setupResult, errorText("unable to launch codex app-server", setupResult)});
                return;
            }

            childPid = spawnedPid;
            try {
                selfKeepAlive = shared_from_this();

                stdinPipe.closeRead();
                stdoutPipe.closeWrite();
                stderrPipe.closeWrite();

                observeChildExit();

                if (core::SNodeC::state() == core::State::STOPPING) {
                    throw std::runtime_error("event-loop shutdown started during app-server launch");
                }

                if (failParentSetupForTests) {
                    throw std::runtime_error("injected parent receiver setup failure");
                }

                int nonBlockingError = 0;
                if (!setNonBlocking(stdinPipe.getWriteFd(), nonBlockingError) ||
                    !setNonBlocking(stdoutPipe.getReadFd(), nonBlockingError) ||
                    !setNonBlocking(stderrPipe.getReadFd(), nonBlockingError)) {
                    throw std::system_error(
                        nonBlockingError, std::generic_category(), "unable to configure parent app-server pipes as nonblocking");
                }

                const std::shared_ptr<Session> currentSession = shared_from_this();
                StdinPipeSource* const newWriter = new StdinPipeSource(stdinPipe.getWriteFd(), currentSession);
                static_cast<void>(stdinPipe.releaseWriteFd());
                stdinWriter = newWriter;
                core::pipe::PipeSource* const writer = stdinWriter;
                writer->setOnError([weakSession, writer](int errorNumber) {
                    if (const std::shared_ptr<Session> session = weakSession.lock()) {
                        session->stdinError(writer, errorNumber);
                    }
                });
                writer->setOnClosed([weakSession, writer]() {
                    if (const std::shared_ptr<Session> session = weakSession.lock()) {
                        session->stdinClosed(writer);
                    }
                });

                OutputPipeSink* const newStdoutReader = new OutputPipeSink(stdoutPipe.getReadFd(), currentSession);
                static_cast<void>(stdoutPipe.releaseReadFd());
                stdoutReader = newStdoutReader;
                core::pipe::PipeSink* const stdoutSink = stdoutReader;
                stdoutSink->setOnData([weakSession](const char* data, std::size_t length) {
                    if (const std::shared_ptr<Session> session = weakSession.lock()) {
                        session->stdoutData(data, length);
                    }
                });
                stdoutSink->setOnEof([weakSession, stdoutSink]() {
                    if (const std::shared_ptr<Session> session = weakSession.lock()) {
                        session->stdoutEof(stdoutSink);
                    }
                });
                stdoutSink->setOnError([weakSession, stdoutSink](int errorNumber) {
                    if (const std::shared_ptr<Session> session = weakSession.lock()) {
                        session->stdoutError(stdoutSink, errorNumber);
                    }
                });
                stdoutSink->setOnClosed([weakSession, stdoutSink]() {
                    if (const std::shared_ptr<Session> session = weakSession.lock()) {
                        session->stdoutClosed(stdoutSink);
                    }
                });

                OutputPipeSink* const newStderrReader = new OutputPipeSink(stderrPipe.getReadFd(), currentSession);
                static_cast<void>(stderrPipe.releaseReadFd());
                stderrReader = newStderrReader;
                core::pipe::PipeSink* const stderrSink = stderrReader;
                stderrSink->setOnData([weakSession](const char* data, std::size_t length) {
                    if (const std::shared_ptr<Session> session = weakSession.lock()) {
                        session->stderrData(data, length);
                    }
                });
                stderrSink->setOnEof([weakSession, stderrSink]() {
                    if (const std::shared_ptr<Session> session = weakSession.lock()) {
                        session->stderrEof(stderrSink);
                    }
                });
                stderrSink->setOnError([weakSession, stderrSink](int errorNumber) {
                    if (const std::shared_ptr<Session> session = weakSession.lock()) {
                        session->stderrError(stderrSink, errorNumber);
                    }
                });
                stderrSink->setOnClosed([weakSession, stderrSink]() {
                    if (const std::shared_ptr<Session> session = weakSession.lock()) {
                        session->stderrClosed(stderrSink);
                    }
                });

                logScope.logger(logger::Logger::semanticSink()).trace("Spawned codex app-server: pid={}", childPid);
                const std::function<void()> callback = callbacks.onStarted;
                if (callback) {
                    callback();
                }
            } catch (const std::exception& exception) {
                stdinPipe.closeRead();
                stdinPipe.closeWrite();
                stdoutPipe.closeRead();
                stdoutPipe.closeWrite();
                stderrPipe.closeRead();
                stderrPipe.closeWrite();
                rollbackSpawn(
                    {Error::Category::Transport, EIO, std::string("unable to configure parent app-server transport: ") + exception.what()});
                return;
            } catch (...) {
                stdinPipe.closeRead();
                stdinPipe.closeWrite();
                stdoutPipe.closeRead();
                stdoutPipe.closeWrite();
                stderrPipe.closeRead();
                stderrPipe.closeWrite();
                rollbackSpawn({Error::Category::Transport, EIO, "unable to configure parent app-server transport"});
                return;
            }
        }

        bool send(std::string message) {
            message.push_back('\n');
            return !stopping && stdinWriter != nullptr && stdinWriter->send(message);
        }

        void stop() {
            if (stopping) {
                return;
            }

            stopping = true;
            if (stdinWriter != nullptr) {
                stopPhase = StopPhase::FlushingStdin;
                stopDeadline = utils::Timeval::currentTime() + STDIN_FLUSH_INTERVAL;
                stdinWriter->eof();
            } else {
                beginSigtermGracePeriod();
            }
        }

        void forceStop() {
            stopping = true;
            stopPhase = StopPhase::Reaping;
            closeEndpoints();
            signalChildProcessGroup(SIGKILL);
            childMayHaveExited(nullptr);
        }

        void eventLoopShutdown() {
            stop();
        }

        void lifecycleEvent() {
            lifecycleTick();
        }

        void childMayHaveExited([[maybe_unused]] ChildExitReceiver* observer) {
            if (childPid <= 0) {
                if (processObserver != nullptr) {
                    processObserver->detach();
                    processObserver = nullptr;
                }
                return;
            }

            int status = 0;
            pid_t waitResult = -1;
            do {
                waitResult = ::waitpid(childPid, &status, WNOHANG);
            } while (waitResult < 0 && errno == EINTR);

            if (waitResult == 0) {
                return;
            }

            codex::detail::ProcessExit exitStatus;
            if (waitResult == childPid) {
                if (WIFEXITED(status)) {
                    exitStatus.exited = true;
                    exitStatus.exitCode = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    exitStatus.signaled = true;
                    exitStatus.signalNumber = WTERMSIG(status);
                }
            } else {
                const int waitError = errno;
                reportError({Error::Category::Process, waitError, errorText("waitpid(WNOHANG) failed", waitError)});
            }

            childPid = -1;
            if (processObserver != nullptr) {
                processObserver->detach();
                processObserver = nullptr;
            }
            finalizeExit(exitStatus);
        }

        void processObserverClosed(ChildExitReceiver* observer) {
            if (processObserver != observer) {
                return;
            }

            processObserver = nullptr;
            if (childPid > 0) {
                forceStop();
            }
        }

        void lifecycleObserverClosed(LifecycleReceiver* observer) {
            if (lifecycleObserver != observer) {
                return;
            }

            lifecycleObserver = nullptr;
            if (childPid > 0) {
                forceStop();
            }
        }

    private:
        enum class StopPhase { Running, FlushingStdin, SigtermGrace, SigkillGrace, Reaping };

        void lifecycleTick() {
            if (childPid <= 0) {
                return;
            }

            childMayHaveExited(nullptr);
            if (childPid <= 0) {
                return;
            }

            if (!stopping) {
                return;
            }

            const utils::Timeval currentTime = utils::Timeval::currentTime();
            if (stopPhase == StopPhase::FlushingStdin && currentTime >= stopDeadline) {
                if (stdinWriter != nullptr) {
                    stdinFlushTimedOut = true;
                    stdinWriter->close();
                } else {
                    sendSigtermAndBeginSigkillGracePeriod();
                }
            } else if (stopPhase == StopPhase::SigtermGrace && currentTime >= stopDeadline) {
                sendSigtermAndBeginSigkillGracePeriod();
            } else if (stopPhase == StopPhase::SigkillGrace && currentTime >= stopDeadline) {
                signalChildProcessGroup(SIGKILL);
                stopPhase = StopPhase::Reaping;
            }
        }

        void beginSigtermGracePeriod() {
            if (childPid <= 0 || stopPhase == StopPhase::SigtermGrace || stopPhase == StopPhase::SigkillGrace ||
                stopPhase == StopPhase::Reaping) {
                return;
            }

            stopPhase = StopPhase::SigtermGrace;
            stopDeadline = utils::Timeval::currentTime() + SIGTERM_GRACE_INTERVAL;
        }

        void sendSigtermAndBeginSigkillGracePeriod() {
            if (childPid <= 0 || stopPhase == StopPhase::SigkillGrace || stopPhase == StopPhase::Reaping) {
                return;
            }

            signalChildProcessGroup(SIGTERM);
            stopPhase = StopPhase::SigkillGrace;
            stopDeadline = utils::Timeval::currentTime() + SIGKILL_GRACE_INTERVAL;
        }

        void signalChildProcessGroup(int signalNumber) {
            if (childPid > 0 && ::kill(-childPid, signalNumber) != 0 && errno == ESRCH) {
                static_cast<void>(::kill(childPid, signalNumber));
            }
        }

        void observeChildExit() noexcept {
            if (!forceChildExitPollingForTests) {
                const int pidfd = relocateAboveStandardDescriptors(openPidFd(childPid));
                if (pidfd >= 0) {
                    try {
                        processObserver = new ChildExitReceiver(pidfd, shared_from_this());
                        return;
                    } catch (...) {
                        core::system::close(pidfd);
                    }
                }
            }
        }

        void rollbackSpawn(Error error) {
            stopping = true;
            stopPhase = StopPhase::Reaping;
            closeEndpoints();
            if (processObserver == nullptr) {
                observeChildExit();
            }
            signalChildProcessGroup(SIGKILL);
            reportError(std::move(error));
            childMayHaveExited(nullptr);
        }

        void stdoutData(const char* data, std::size_t length) {
            std::string framingError;
            const bool framed = stdoutFramer.feed(
                data,
                length,
                [this](std::string line) {
                    if (line.empty()) {
                        return true;
                    }

                    const std::function<void(std::string)> callback = callbacks.onMessage;
                    if (callback) {
                        callback(std::move(line));
                    }
                    return !stopping;
                },
                framingError);

            if (!framed && !framingError.empty()) {
                reportError({Error::Category::Protocol, 0, std::move(framingError)});
            }
        }

        void stderrData(const char* data, std::size_t length) {
            std::string framingError;
            const bool framed = stderrFramer.feed(
                data,
                length,
                [this](std::string line) {
                    emitDiagnostic(std::move(line));
                    return !stopping;
                },
                framingError);

            if (!framed && !framingError.empty()) {
                emitDiagnostic("codex app-server stderr " + framingError);
                core::pipe::PipeSink* const reader = stderrReader;
                stderrReader = nullptr;
                if (reader != nullptr) {
                    reader->close();
                }
            }
        }

        void stdoutEof(core::pipe::PipeSink* reader) {
            if (stdoutReader == reader) {
                stdoutReader = nullptr;
            }
            static_cast<void>(stdoutFramer.finish([this](std::string line) {
                const std::function<void(std::string)> callback = callbacks.onMessage;
                if (callback && !line.empty()) {
                    callback(std::move(line));
                }
                return !stopping;
            }));

            deferUnexpectedClosure("stdout");
        }

        void stderrEof(core::pipe::PipeSink* reader) {
            if (stderrReader == reader) {
                stderrReader = nullptr;
            }
            static_cast<void>(stderrFramer.finish([this](std::string line) {
                emitDiagnostic(std::move(line));
                return !stopping;
            }));
        }

        void stdoutError(core::pipe::PipeSink* reader, int errnum) {
            if (stdoutReader == reader) {
                stdoutReader = nullptr;
            }
            if (!stopping) {
                deferErrorUntilExitCheck({Error::Category::Transport, errnum, errorText("codex app-server stdout read failed", errnum)});
            }
        }

        void stderrError(core::pipe::PipeSink* reader, int errnum) {
            if (stderrReader == reader) {
                stderrReader = nullptr;
            }
            static_cast<void>(stderrFramer.finish([this](std::string line) {
                emitDiagnostic(std::move(line));
                return true;
            }));
            emitDiagnostic(errorText("codex app-server stderr read failed", errnum));
        }

        void stdoutClosed(core::pipe::PipeSink* reader) {
            if (stdoutReader == reader) {
                stdoutReader = nullptr;
            }
        }

        void stderrClosed(core::pipe::PipeSink* reader) {
            if (stderrReader == reader) {
                stderrReader = nullptr;
            }
        }

        void stdinError(core::pipe::PipeSource* writer, int errnum) {
            if (stdinWriter == writer) {
                stdinWriter = nullptr;
            }
            if (!stopping) {
                deferErrorUntilExitCheck({Error::Category::Transport, errnum, errorText("codex app-server stdin write failed", errnum)});
            }
        }

        void stdinClosed(core::pipe::PipeSource* writer) {
            if (stdinWriter == writer) {
                stdinWriter = nullptr;
            }
            if (stopping && stopPhase == StopPhase::FlushingStdin) {
                if (stdinFlushTimedOut) {
                    sendSigtermAndBeginSigkillGracePeriod();
                } else {
                    beginSigtermGracePeriod();
                }
            }
        }

        void deferUnexpectedClosure(std::string channel) {
            if (stopping) {
                return;
            }

            deferErrorUntilExitCheck({Error::Category::Transport, 0, "codex app-server " + channel + " closed unexpectedly"});
        }

        void deferErrorUntilExitCheck(Error error) {
            const std::weak_ptr<Session> weakSession = weak_from_this();
            static_cast<void>(core::timer::Timer::singleshotTimer(
                [weakSession, error = std::move(error)]() mutable {
                    if (const std::shared_ptr<Session> session = weakSession.lock();
                        session && !session->stopping && session->childPid > 0) {
                        // Descriptor publishers are allowed to deliver pipe EOF or
                        // EPIPE before pidfd readability. The short event-loop delay
                        // gives an exiting child priority over a secondary channel
                        // error while still reporting a live child's closed channel.
                        session->childMayHaveExited(session->processObserver);
                        if (session->childPid > 0) {
                            session->reportError(std::move(error));
                        }
                    }
                },
                CHILD_EXIT_POLL_INTERVAL));
        }

        void emitDiagnostic(std::string line) {
            const std::function<void(Diagnostic)> callback = callbacks.onDiagnostic;
            if (callback) {
                callback({std::move(line)});
            }
        }

        void reportError(Error error) {
            if (errorReported) {
                return;
            }
            errorReported = true;

            logScope.logger(logger::Logger::semanticSink()).error("Codex stdio transport failed: {}", error.message);
            const std::function<void(Error)> callback = callbacks.onError;
            if (callback) {
                callback(std::move(error));
            }
        }

        void finalizeExit(const codex::detail::ProcessExit& status) {
            const std::shared_ptr<Session> keepAlive = shared_from_this();
            closeEndpoints();
            detachLifecycleObserver();

            const std::function<void(codex::detail::ProcessExit)> callback = callbacks.onExited;
            selfKeepAlive.reset();
            if (callback) {
                callback(status);
            }
        }

        void closeWriter() {
            if (stdinWriter != nullptr) {
                stdinWriter->setOnError({});
                stdinWriter->setOnClosed({});
                stdinWriter->close();
                stdinWriter = nullptr;
            }
        }

        static void closeReader(core::pipe::PipeSink*& reader) {
            if (reader != nullptr) {
                reader->setOnData({});
                reader->setOnEof({});
                reader->setOnError({});
                reader->setOnClosed({});
                reader->close();
                reader = nullptr;
            }
        }

        void closeEndpoints() {
            closeWriter();
            closeReader(stdoutReader);
            closeReader(stderrReader);
        }

        void detachLifecycleObserver() {
            if (lifecycleObserver != nullptr) {
                lifecycleObserver->detach();
                lifecycleObserver = nullptr;
            }
        }

        std::string executable;
        std::vector<std::string> arguments;
        bool forceChildExitPollingForTests;
        bool failParentSetupForTests;
        codex::detail::TransportCallbacks callbacks;

        pid_t childPid = -1;
        core::pipe::PipeSource* stdinWriter = nullptr;
        core::pipe::PipeSink* stdoutReader = nullptr;
        core::pipe::PipeSink* stderrReader = nullptr;
        LifecycleReceiver* lifecycleObserver = nullptr;
        ChildExitReceiver* processObserver = nullptr;

        LineFramer stdoutFramer;
        LineFramer stderrFramer;
        bool stopping = false;
        bool errorReported = false;
        bool stdinFlushTimedOut = false;
        StopPhase stopPhase = StopPhase::Running;
        utils::Timeval stopDeadline;

        std::shared_ptr<Session> selfKeepAlive;
        logger::LogScopeOwner logScope;

        friend class ChildExitReceiver;
        friend class LifecycleReceiver;
    };

    namespace {
        StdinPipeSource::StdinPipeSource(int fd, const std::shared_ptr<StdioTransport::Session>& session)
            : core::pipe::PipeSource(fd, DEFAULT_MAX_QUEUED_BYTES, TIMEOUT::DISABLE)
            , session(session) {
        }

        void StdinPipeSource::onShutdown(const core::ShutdownContext&) {
            if (const std::shared_ptr<StdioTransport::Session> currentSession = session.lock()) {
                currentSession->eventLoopShutdown();
            }
        }

        OutputPipeSink::OutputPipeSink(int fd, const std::shared_ptr<StdioTransport::Session>& session)
            : core::pipe::PipeSink(fd, DEFAULT_MAX_BYTES_PER_EVENT, TIMEOUT::DISABLE)
            , session(session) {
        }

        void OutputPipeSink::onShutdown(const core::ShutdownContext&) {
            if (const std::shared_ptr<StdioTransport::Session> currentSession = session.lock()) {
                currentSession->eventLoopShutdown();
            }
        }

        LifecycleReceiver::LifecycleReceiver(int readFd, int writeFd, const std::shared_ptr<StdioTransport::Session>& session)
            : core::eventreceiver::ReadEventReceiver("codex app-server lifecycle", CHILD_EXIT_POLL_INTERVAL)
            , session(session)
            , writeFd(writeFd) {
            if (!ReadEventReceiver::enable(readFd)) {
                throw std::runtime_error("unable to register codex app-server lifecycle receiver");
            }
        }

        LifecycleReceiver::~LifecycleReceiver() {
            core::system::close(getRegisteredFd());
            core::system::close(writeFd);
        }

        void LifecycleReceiver::detach() {
            session.reset();
            if (ReadEventReceiver::isEnabled()) {
                ReadEventReceiver::disable();
            }
        }

        void LifecycleReceiver::readEvent() {
            const std::shared_ptr<StdioTransport::Session> currentSession = session;
            if (currentSession) {
                currentSession->lifecycleEvent();
            }
        }

        void LifecycleReceiver::readTimeout() {
            setTimeout(CHILD_EXIT_POLL_INTERVAL);
            const std::shared_ptr<StdioTransport::Session> currentSession = session;
            if (currentSession) {
                currentSession->lifecycleEvent();
            }
        }

        void LifecycleReceiver::unobservedEvent() {
            const std::shared_ptr<StdioTransport::Session> currentSession = std::move(session);
            if (currentSession) {
                currentSession->lifecycleObserverClosed(this);
            }
            delete this;
        }

        void LifecycleReceiver::onShutdown(const core::ShutdownContext&) {
            const std::shared_ptr<StdioTransport::Session> currentSession = session;
            if (currentSession) {
                currentSession->eventLoopShutdown();
            }
        }

        ChildExitReceiver::ChildExitReceiver(int pidfd, const std::shared_ptr<StdioTransport::Session>& session)
            : core::eventreceiver::ReadEventReceiver("codex app-server pidfd", TIMEOUT::DISABLE)
            , session(session) {
            if (!ReadEventReceiver::enable(pidfd)) {
                throw std::runtime_error("unable to register codex app-server pidfd");
            }
        }

        ChildExitReceiver::~ChildExitReceiver() {
            core::system::close(getRegisteredFd());
        }

        void ChildExitReceiver::detach() {
            session.reset();
            if (ReadEventReceiver::isEnabled()) {
                ReadEventReceiver::disable();
            }
        }

        void ChildExitReceiver::readEvent() {
            const std::shared_ptr<StdioTransport::Session> currentSession = session;
            if (currentSession) {
                currentSession->childMayHaveExited(this);
            }
        }

        void ChildExitReceiver::unobservedEvent() {
            const std::shared_ptr<StdioTransport::Session> currentSession = std::move(session);
            if (currentSession) {
                currentSession->processObserverClosed(this);
            }
            delete this;
        }

        void ChildExitReceiver::onShutdown(const core::ShutdownContext&) {
            const std::shared_ptr<StdioTransport::Session> currentSession = session;
            if (currentSession) {
                currentSession->eventLoopShutdown();
            }
        }
    } // namespace

    StdioTransport::StdioTransport(std::string executable,
                                   std::vector<std::string> arguments,
                                   bool forceChildExitPollingForTests,
                                   bool failParentSetupForTests)
        : executable(std::move(executable))
        , arguments(std::move(arguments))
        , forceChildExitPollingForTests(forceChildExitPollingForTests)
        , failParentSetupForTests(failParentSetupForTests) {
    }

    StdioTransport::~StdioTransport() {
        callbacks = {};
        if (session) {
            session->setCallbacks({});
            session->stop();
            session.reset();
        }
    }

    void StdioTransport::setCallbacks(codex::detail::TransportCallbacks callbacks) {
        this->callbacks = std::move(callbacks);
        if (session) {
            session->setCallbacks(this->callbacks);
        }
    }

    void StdioTransport::start() {
        if (session) {
            session->setCallbacks({});
            session->stop();
        }

        session = std::make_shared<Session>(executable, arguments, forceChildExitPollingForTests, failParentSetupForTests, callbacks);
        session->start();
    }

    bool StdioTransport::send(std::string message) {
        return session && session->send(std::move(message));
    }

    void StdioTransport::stop() {
        if (session) {
            session->stop();
        }
    }

} // namespace ai::openai::codex::stdio::detail
