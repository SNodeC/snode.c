#ifndef CORE_SOCKET_STREAM_TLS_DETAIL_TLSLIFECYCLETESTHOOKS_H
#define CORE_SOCKET_STREAM_TLS_DETAIL_TLSLIFECYCLETESTHOOKS_H

#include <cerrno>
#include <deque>
#include <string>

namespace core::socket::stream::tls::detail::test {

    enum class Axis { None, Read, Write };
    enum class Outcome { Success, Timeout, Error, WantRead, WantWrite };

    struct Counters {
        int constructed = 0;
        int destroyed = 0;
        int active = 0;
        int maxConcurrent = 0;
        int operationCalls = 0;
        int readEnables = 0;
        int writeEnables = 0;
        int readResumes = 0;
        int writeResumes = 0;
        int readSuspends = 0;
        int writeSuspends = 0;
        int readDisables = 0;
        int writeDisables = 0;
        int successes = 0;
        int timeouts = 0;
        int errors = 0;
        int releases = 0;
        int lastErrno = 0;
    };

    class LifecycleModel {
    public:
        explicit LifecycleModel(Counters& counters)
            : counters(counters) {
            counters.constructed++;
            counters.active++;
            if (counters.active > counters.maxConcurrent) {
                counters.maxConcurrent = counters.active;
            }
        }

        ~LifecycleModel() {
            counters.destroyed++;
            counters.active--;
        }

        LifecycleModel(const LifecycleModel&) = delete;
        LifecycleModel& operator=(const LifecycleModel&) = delete;

        void enqueue(Outcome outcome) { outcomes.push_back(outcome); }
        void failNextReadEnable(int error) { readEnableError = error; }
        void failNextWriteEnable(int error) { writeEnableError = error; }

        void start() {
            if (completed) {
                return;
            }
            perform();
        }

        void readEvent() {
            if (completed) {
                return;
            }
            perform();
        }

        void writeEvent() {
            if (completed) {
                return;
            }
            perform();
        }

        void readTimeout() {
            if (completed) {
                return;
            }
            finishTimeout();
        }

        void writeTimeout() {
            if (completed) {
                return;
            }
            finishTimeout();
        }

        void unobservedEvent() {
            notifyReleased();
            delete this;
        }

        bool isCompleted() const { return completed; }
        bool isReleased() const { return releaseNotified; }
        bool wasEverObserved() const { return everObserved; }
        bool guardActive() const { return !releaseNotified; }
        Axis activeAxis() const { return active; }
        bool readRegisteredAxis() const { return readRegistered; }
        bool writeRegisteredAxis() const { return writeRegistered; }

    private:
        void perform() {
            if (completed) {
                return;
            }
            counters.operationCalls++;
            const Outcome outcome = outcomes.empty() ? Outcome::Success : outcomes.front();
            if (!outcomes.empty()) {
                outcomes.pop_front();
            }
            switch (outcome) {
                case Outcome::Success:
                    finishSuccess();
                    break;
                case Outcome::Timeout:
                    finishTimeout();
                    break;
                case Outcome::Error:
                    finishError(EIO);
                    break;
                case Outcome::WantRead:
                    awaitRead();
                    break;
                case Outcome::WantWrite:
                    awaitWrite();
                    break;
            }
        }

        void awaitRead() {
            if (completed) {
                return;
            }
            if (active == Axis::Write) {
                counters.writeSuspends++;
            }
            if (!readRegistered) {
                if (readEnableError != 0) {
                    const int error = readEnableError;
                    readEnableError = 0;
                    finishError(error);
                    return;
                }
                readRegistered = true;
                everObserved = true;
                counters.readEnables++;
            }
            if (active != Axis::Read) {
                counters.readResumes++;
            }
            active = Axis::Read;
        }

        void awaitWrite() {
            if (completed) {
                return;
            }
            if (active == Axis::Read) {
                counters.readSuspends++;
            }
            if (!writeRegistered) {
                if (writeEnableError != 0) {
                    const int error = writeEnableError;
                    writeEnableError = 0;
                    finishError(error);
                    return;
                }
                writeRegistered = true;
                everObserved = true;
                counters.writeEnables++;
            }
            if (active != Axis::Write) {
                counters.writeResumes++;
            }
            active = Axis::Write;
        }

        void finishSuccess() {
            if (completed) {
                return;
            }
            completed = true;
            disableRegistered();
            counters.successes++;
            releaseIfNeverObserved();
        }

        void finishTimeout() {
            if (completed) {
                return;
            }
            completed = true;
            disableRegistered();
            counters.timeouts++;
            releaseIfNeverObserved();
        }

        void finishError(int error) {
            if (completed) {
                return;
            }
            completed = true;
            counters.lastErrno = error;
            disableRegistered();
            counters.errors++;
            releaseIfNeverObserved();
        }

        void disableRegistered() {
            if (readRegistered) {
                counters.readDisables++;
            }
            if (writeRegistered) {
                counters.writeDisables++;
            }
            active = Axis::None;
        }

        void releaseIfNeverObserved() {
            if (!everObserved) {
                notifyReleased();
                delete this;
            }
        }

        void notifyReleased() {
            if (!releaseNotified) {
                releaseNotified = true;
                counters.releases++;
            }
        }

        Counters& counters;
        std::deque<Outcome> outcomes;
        Axis active = Axis::None;
        bool completed = false;
        bool everObserved = false;
        bool releaseNotified = false;
        bool readRegistered = false;
        bool writeRegistered = false;
        int readEnableError = 0;
        int writeEnableError = 0;
    };

} // namespace core::socket::stream::tls::detail::test

#endif
