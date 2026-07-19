/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/stdio/StdioTransport.h"

#include "ai/openai/codex/AppServerClient.h"
#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/pipe/PipeSink.h"
#include "core/pipe/PipeSource.h"
#include "core/system/unistd.h"
#include "core/timer/Timer.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <memory>
#include <spawn.h>
#include <stdlib.h>
#include <string>
#include <sys/wait.h>
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
        const utils::Timeval GRACEFUL_STOP_INTERVAL = {0, 250000};
        const utils::Timeval TERMINATE_STOP_INTERVAL = {0, 250000};

        class OwnedFd {
        public:
            OwnedFd() = default;

            explicit OwnedFd(int fd)
                : fd(fd) {
            }

            OwnedFd(const OwnedFd&) = delete;

            OwnedFd(OwnedFd&& other) noexcept
                : fd(std::exchange(other.fd, -1)) {
            }

            OwnedFd& operator=(const OwnedFd&) = delete;

            OwnedFd& operator=(OwnedFd&& other) noexcept {
                reset();
                fd = std::exchange(other.fd, -1);
                return *this;
            }

            ~OwnedFd() {
                reset();
            }

            int get() const noexcept {
                return fd;
            }

            int release() noexcept {
                return std::exchange(fd, -1);
            }

            void reset(int replacement = -1) noexcept {
                if (fd >= 0) {
                    core::system::close(fd);
                }
                fd = replacement;
            }

        private:
            int fd = -1;
        };

        struct PipePair {
            OwnedFd readEnd;
            OwnedFd writeEnd;
        };

        int normalizeDescriptor(int fd) {
            if (fd >= STDERR_FILENO + 1) {
                return fd;
            }

            const int replacement = ::fcntl(fd, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
            const int savedErrno = errno;
            core::system::close(fd);
            errno = savedErrno;
            return replacement;
        }

        bool makePipe(PipePair& pipe, int& errnum) {
            int descriptors[2] = {-1, -1};
            if (core::system::pipe2(descriptors, O_CLOEXEC) != 0) {
                errnum = errno;
                return false;
            }

            descriptors[0] = normalizeDescriptor(descriptors[0]);
            if (descriptors[0] < 0) {
                errnum = errno;
                core::system::close(descriptors[1]);
                return false;
            }

            descriptors[1] = normalizeDescriptor(descriptors[1]);
            if (descriptors[1] < 0) {
                errnum = errno;
                core::system::close(descriptors[0]);
                return false;
            }

            pipe.readEnd.reset(descriptors[0]);
            pipe.writeEnd.reset(descriptors[1]);
            return true;
        }

        bool setNonBlocking(int fd, int& errnum) {
            const int flags = ::fcntl(fd, F_GETFL);
            if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
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
    } // namespace

    namespace {
        class ChildExitReceiver final : public core::eventreceiver::ReadEventReceiver {
        public:
            ChildExitReceiver(int pidfd, const std::shared_ptr<StdioTransport::Session>& session);
            ~ChildExitReceiver() override;

            void detach();

        private:
            void readEvent() override;
            void unobservedEvent() override;

            std::shared_ptr<StdioTransport::Session> session;
        };
    } // namespace

    class StdioTransport::Session : public std::enable_shared_from_this<StdioTransport::Session> {
    public:
        Session(std::string executable, std::vector<std::string> arguments, codex::detail::TransportCallbacks callbacks)
            : executable(std::move(executable))
            , arguments(std::move(arguments))
            , callbacks(std::move(callbacks))
            , logScope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "ai.openai.codex.stdio") {
        }

        ~Session() {
            closeEndpoints();
            if (processObserver != nullptr) {
                processObserver->detach();
                processObserver = nullptr;
            }
        }

        void setCallbacks(codex::detail::TransportCallbacks callbacks) {
            this->callbacks = std::move(callbacks);
        }

        void start() {
            PipePair stdinPipe;
            PipePair stdoutPipe;
            PipePair stderrPipe;
            int errnum = 0;

            if (!makePipe(stdinPipe, errnum) || !makePipe(stdoutPipe, errnum) || !makePipe(stderrPipe, errnum) ||
                !setNonBlocking(stdinPipe.writeEnd.get(), errnum) || !setNonBlocking(stdoutPipe.readEnd.get(), errnum) ||
                !setNonBlocking(stderrPipe.readEnd.get(), errnum)) {
                reportError({Error::Category::Launch, errnum, errorText("unable to create app-server pipes", errnum)});
                return;
            }

            posix_spawn_file_actions_t fileActions{};
            const int actionsInitResult = posix_spawn_file_actions_init(&fileActions);
            if (actionsInitResult != 0) {
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

            addAction(posix_spawn_file_actions_adddup2(&fileActions, stdinPipe.readEnd.get(), STDIN_FILENO));
            addAction(posix_spawn_file_actions_adddup2(&fileActions, stdoutPipe.writeEnd.get(), STDOUT_FILENO));
            addAction(posix_spawn_file_actions_adddup2(&fileActions, stderrPipe.writeEnd.get(), STDERR_FILENO));
            for (const int fd : {stdinPipe.readEnd.get(),
                                 stdinPipe.writeEnd.get(),
                                 stdoutPipe.readEnd.get(),
                                 stdoutPipe.writeEnd.get(),
                                 stderrPipe.readEnd.get(),
                                 stderrPipe.writeEnd.get()}) {
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
            if (setupResult == 0) {
                std::vector<char*> argv = buildArgv(executable, arguments);
                setupResult = posix_spawnp(&spawnedPid, executable.c_str(), &fileActions, &attributes, argv.data(), environ);
            }

            if (attributesInitResult == 0) {
                static_cast<void>(posix_spawnattr_destroy(&attributes));
            }
            static_cast<void>(posix_spawn_file_actions_destroy(&fileActions));

            if (setupResult != 0) {
                reportError({Error::Category::Launch, setupResult, errorText("unable to launch codex app-server", setupResult)});
                return;
            }

            childPid = spawnedPid;
            selfKeepAlive = shared_from_this();

            stdinPipe.readEnd.reset();
            stdoutPipe.writeEnd.reset();
            stderrPipe.writeEnd.reset();

            const std::weak_ptr<Session> weakSession = weak_from_this();

            stdinWriter = new core::pipe::PipeSource(
                stdinPipe.writeEnd.release(), core::pipe::PipeSource::DEFAULT_MAX_QUEUED_BYTES, core::pipe::PipeSource::TIMEOUT::DISABLE);
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

            stdoutReader = new core::pipe::PipeSink(
                stdoutPipe.readEnd.release(), core::pipe::PipeSink::DEFAULT_MAX_BYTES_PER_EVENT, core::pipe::PipeSink::TIMEOUT::DISABLE);
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

            stderrReader = new core::pipe::PipeSink(
                stderrPipe.readEnd.release(), core::pipe::PipeSink::DEFAULT_MAX_BYTES_PER_EVENT, core::pipe::PipeSink::TIMEOUT::DISABLE);
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

            const int pidfd = openPidFd(childPid);
            if (pidfd >= 0) {
                processObserver = new ChildExitReceiver(pidfd, shared_from_this());
            } else {
                scheduleChildPoll();
            }

            logScope.logger(logger::Logger::semanticSink()).trace("Spawned codex app-server: pid={}", childPid);
            const std::function<void()> callback = callbacks.onStarted;
            if (callback) {
                callback();
            }
        }

        bool send(std::string message) {
            return !stopping && stdinWriter != nullptr && stdinWriter->trySend(message);
        }

        void stop() {
            if (stopping) {
                return;
            }

            stopping = true;
            requestWriterEof();
            if (childPid > 0) {
                scheduleSignal(SIGTERM, GRACEFUL_STOP_INTERVAL, [weakSession = weak_from_this()]() {
                    if (const std::shared_ptr<Session> session = weakSession.lock()) {
                        session->scheduleSignal(SIGKILL, TERMINATE_STOP_INTERVAL, {});
                    }
                });
            }
        }

        void childMayHaveExited(ChildExitReceiver* observer) {
            if (childPid <= 0) {
                if (observer != nullptr) {
                    observer->detach();
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
                if (observer == nullptr) {
                    scheduleChildPoll();
                }
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
            if (observer != nullptr) {
                observer->detach();
                processObserver = nullptr;
            }

            const std::weak_ptr<Session> weakSession = weak_from_this();
            core::EventReceiver::atNextTick([weakSession, exitStatus]() {
                if (const std::shared_ptr<Session> session = weakSession.lock()) {
                    session->finalizeExit(exitStatus);
                }
            });
        }

        void processObserverClosed(ChildExitReceiver* observer) {
            if (processObserver != observer) {
                return;
            }

            processObserver = nullptr;
            if (childPid > 0) {
                static_cast<void>(::kill(-childPid, SIGKILL));
                pollChildAtNextTick = true;
                scheduleChildPoll();
            }
        }

    private:
        void scheduleChildPoll() {
            if (childPid <= 0 || childPollScheduled) {
                return;
            }

            childPollScheduled = true;
            const std::weak_ptr<Session> weakSession = weak_from_this();
            const auto poll = [weakSession]() {
                if (const std::shared_ptr<Session> session = weakSession.lock()) {
                    session->childPollScheduled = false;
                    session->childMayHaveExited(nullptr);
                }
            };

            if (pollChildAtNextTick) {
                core::EventReceiver::atNextTick(poll);
            } else {
                static_cast<void>(core::timer::Timer::singleshotTimer(poll, CHILD_EXIT_POLL_INTERVAL));
            }
        }

        void scheduleSignal(int signalNumber, const utils::Timeval& delay, std::function<void()> afterSignal) {
            const std::weak_ptr<Session> weakSession = weak_from_this();
            static_cast<void>(core::timer::Timer::singleshotTimer(
                [weakSession, signalNumber, afterSignal = std::move(afterSignal)]() {
                    if (const std::shared_ptr<Session> session = weakSession.lock()) {
                        if (session->childPid > 0) {
                            if (::kill(-session->childPid, signalNumber) != 0 && errno == ESRCH) {
                                static_cast<void>(::kill(session->childPid, signalNumber));
                            }
                            if (afterSignal) {
                                afterSignal();
                            }
                        }
                    }
                },
                delay));
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
                reportError({Error::Category::Transport, 0, "stderr " + framingError});
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

            deferUnexpectedClosure("stderr");
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
            if (!stopping) {
                deferErrorUntilExitCheck({Error::Category::Transport, errnum, errorText("codex app-server stderr read failed", errnum)});
            }
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

            const std::function<void(codex::detail::ProcessExit)> callback = callbacks.onExited;
            selfKeepAlive.reset();
            if (callback) {
                callback(status);
            }
        }

        void requestWriterEof() {
            if (stdinWriter != nullptr) {
                stdinWriter->eof();
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

        std::string executable;
        std::vector<std::string> arguments;
        codex::detail::TransportCallbacks callbacks;

        pid_t childPid = -1;
        core::pipe::PipeSource* stdinWriter = nullptr;
        core::pipe::PipeSink* stdoutReader = nullptr;
        core::pipe::PipeSink* stderrReader = nullptr;
        ChildExitReceiver* processObserver = nullptr;

        LineFramer stdoutFramer;
        LineFramer stderrFramer;
        bool stopping = false;
        bool errorReported = false;
        bool childPollScheduled = false;
        bool pollChildAtNextTick = false;

        std::shared_ptr<Session> selfKeepAlive;
        logger::LogScopeOwner logScope;

        friend class ChildExitReceiver;
    };

    namespace {
        ChildExitReceiver::ChildExitReceiver(int pidfd, const std::shared_ptr<StdioTransport::Session>& session)
            : core::eventreceiver::ReadEventReceiver("codex app-server pidfd", TIMEOUT::DISABLE)
            , session(session) {
            ReadEventReceiver::enable(pidfd);
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
    } // namespace

    StdioTransport::StdioTransport(std::string executable, std::vector<std::string> arguments)
        : executable(std::move(executable))
        , arguments(std::move(arguments)) {
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

        session = std::make_shared<Session>(executable, arguments, callbacks);
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
