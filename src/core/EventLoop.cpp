/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "core/EventLoop.h" // for EventLoop

#include "core/DynamicLoader.h"
#include "core/EventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/signal.h"
#include "log/Logger.h" // for Logger
#include "utils/Config.h"

#include <cstdlib> // for exit
#include <cstring>
#include <string> // for string, to_string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_READ_INACTIVITY 60
#define MAX_WRITE_INACTIVITY 60
#define MAX_OUTOFBAND_INACTIVITY 60

/* Must be implemented in every variant of a multiplexer api */
core::EventDispatcher& EventDispatcher();

namespace core {

    core::EventDispatcher& EventLoop::eventDispatcher = ::EventDispatcher();

    bool EventLoop::initialized = false;
    bool EventLoop::running = false;
    bool EventLoop::stopped = true;
    int EventLoop::stopsig = 0;
    unsigned long EventLoop::tickCounter = 0;

    bool EventLoop::dumpConfig = false;

    int daemonize(std::string name, std::string path = "", std::string outfile = "", std::string errfile = "", std::string infile = "") {
        if (path.empty()) {
            path = "/";
        }
        if (name.empty()) {
            name = "SNodeC daemon";
        }
        if (infile.empty()) {
            infile = "/dev/null";
        }
        if (outfile.empty()) {
            outfile = "/dev/null";
        }
        if (errfile.empty()) {
            errfile = "/dev/null";
        }
        // printf("%s %s %s %s\n",name,path,outfile,infile);

        pid_t child;
        // fork, detach from process group leader
        if ((child = fork()) < 0) { // failed fork
            VLOG(0) << "error: failed fork";
            exit(EXIT_FAILURE);
        }
        if (child > 0) { // parent
            exit(EXIT_SUCCESS);
        }
        if (setsid() < 0) { // failed to become session leader
            VLOG(0) << "error: failed setsid";
            exit(EXIT_FAILURE);
        }

        // catch/ignore signals
        signal(SIGCHLD, SIG_IGN);
        signal(SIGHUP, SIG_IGN);

        // fork second time
        if ((child = fork()) < 0) { // failed fork
            VLOG(0) << "error: failed fork";
            exit(EXIT_FAILURE);
        }
        if (child > 0) { // parent
            exit(EXIT_SUCCESS);
        }

        // new file permissions
        umask(0);
        // change to path directory
        chdir(path.c_str());

        // Close all open file descriptors
        /*
                long fd;
                for (fd = sysconf(_SC_OPEN_MAX); fd >= 0; --fd) {
                    close(static_cast<int>(fd));
                }

                // reopen stdin, stdout, stderr

                stdin = fopen(infile.c_str(), "r");    // fd=0
                stdout = fopen(outfile.c_str(), "w+"); // fd=1
                stderr = fopen(errfile.c_str(), "w+"); // fd=2
        */
        return (0);
    }

    static std::string getTickCounterAsString(const el::LogMessage*) {
        std::string tick = std::to_string(EventLoop::getTickCounter());

        if (tick.length() < 10) {
            tick.insert(0, 10 - tick.length(), '0');
        }

        return tick;
    }

    unsigned long EventLoop::getTickCounter() {
        return tickCounter;
    }

    EventDispatcher& EventLoop::getEventDispatcher() {
        return eventDispatcher;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    void EventLoop::init(int argc, char* argv[]) {
        logger::Logger::init(argc, argv);

        logger::Logger::setCustomFormatSpec("%tick", core::getTickCounterAsString);

        EventLoop::initialized = true;

        utils::Config::instance().init(std::string(basename(argv[0])), argc, argv);

        if (utils::Config::instance().daemonize()) {
            VLOG(0) << "Daemonizing";
            daemonize(std::string(basename(argv[0])));
        } else {
            VLOG(0) << "Not daemonizing";
        }

        if (utils::Config::instance().kill()) {
            // kill daemon
            VLOG(0) << "Daemon killed";

            exit(0);
        }

        if (!utils::Config::instance().getLogFile().empty()) {
            VLOG(0) << "LogFile: " << utils::Config::instance().getLogFile();

            logger::Logger::logToFile(utils::Config::instance().getLogFile());
            logger::Logger::quiet();
        }
    }

    TickStatus EventLoop::_tick(const utils::Timeval& tickTimeOut, bool stopped) {
        tickCounter++;

        TickStatus tickStatus = eventDispatcher.dispatch(tickTimeOut, stopped);

        DynamicLoader::execDlCloseDeleyed();

        return tickStatus;
    }

    TickStatus EventLoop::tick(const utils::Timeval& timeOut) {
        if (!initialized) {
            PLOG(ERROR) << "snode.c not initialized. Use SNodeC::init(argc, argv) before SNodeC::tick().";
            exit(1);
        }

        sighandler_t oldSigPipeHandler = core::system::signal(SIGPIPE, SIG_IGN);

        TickStatus tickStatus = EventLoop::_tick(timeOut, stopped);

        core::system::signal(SIGPIPE, oldSigPipeHandler);

        return tickStatus;
    }

    int EventLoop::start(const utils::Timeval& timeOut) {
        if (!initialized) {
            PLOG(ERROR) << "snode.c not initialized. Use SNodeC::init(argc, argv) before SNodeC::start().";
            exit(1);
        }

        utils::Config::instance().finish();

        sighandler_t oldSigPipeHandler = core::system::signal(SIGPIPE, SIG_IGN);
        sighandler_t oldSigQuitHandler = core::system::signal(SIGQUIT, EventLoop::stoponsig);
        sighandler_t oldSigHubHandler = core::system::signal(SIGHUP, EventLoop::stoponsig);
        sighandler_t oldSigIntHandler = core::system::signal(SIGINT, EventLoop::stoponsig);
        sighandler_t oldSigTermHandler = core::system::signal(SIGTERM, EventLoop::stoponsig);
        sighandler_t oldSigAbrtHandler = core::system::signal(SIGABRT, EventLoop::stoponsig);

        if (!running) {
            running = true;

            stopped = false;

            core::TickStatus tickStatus = TickStatus::SUCCESS;
            while (tickStatus == TickStatus::SUCCESS && !stopped) {
                tickStatus = EventLoop::_tick(timeOut, stopped);
            }

            switch (tickStatus) {
                case TickStatus::SUCCESS:
                    LOG(INFO) << "EventLoop: exiting - freeing resources";
                    break;
                case TickStatus::NO_OBSERVER:
                    LOG(INFO) << "EventLoop: No Observer - exiting";
                    break;
                case TickStatus::ERROR:
                    PLOG(ERROR) << "EventDispatcher::dispatch()";
                    break;
            }

            running = false;
        }

        core::system::signal(SIGPIPE, oldSigPipeHandler);
        core::system::signal(SIGQUIT, oldSigQuitHandler);
        core::system::signal(SIGHUP, oldSigHubHandler);
        core::system::signal(SIGINT, oldSigIntHandler);
        core::system::signal(SIGTERM, oldSigTermHandler);
        core::system::signal(SIGABRT, oldSigAbrtHandler);

        free();

        int returnReason = 0;
        if (stopsig != 0) {
            returnReason = -stopsig;
        }

        return returnReason;
    }

    void EventLoop::free() {
        core::TickStatus tickStatus;

        do {
            eventDispatcher.stopDescriptorEvents();
            tickStatus = _tick(2, true);
        } while (tickStatus == TickStatus::SUCCESS);

        do {
            eventDispatcher.stopTimerEvents();
            tickStatus = _tick(0, false);
        } while (tickStatus == TickStatus::SUCCESS);

        DynamicLoader::execDlCloseDeleyed();
        DynamicLoader::execDlCloseAll();
    }

    void EventLoop::stoponsig(int sig) {
        stopsig = sig;
        stopped = true;
    }

} // namespace core
