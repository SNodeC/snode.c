// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/system/time.h"

#include <functional>
#include <string>

namespace {
    bool freezeClock = false;
    timeval frozenTime{};
} // namespace

// Interpose the existing clock boundary only in this test executable. Timer
// creation gets identical deadlines without changing the production clock/API.
namespace utils::system {
    int gettimeofday(struct timeval* tv, struct timezone* tz) {
        const int result = ::gettimeofday(tv, tz);
        if (freezeClock) {
            *tv = frozenTime;
        }
        return result;
    }
} // namespace utils::system

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("TimerEqualDeadlineTest");
    } else {
        core::SNodeC::init(argc, argv);
        int first = 0, second = 0, cancelled = 0;
        ::gettimeofday(&frozenTime, nullptr);
        freezeClock = true;
        core::timer::Timer one = core::timer::Timer::singleshotTimer(
            [&first] {
                ++first;
            },
            utils::Timeval(0.01));
        core::timer::Timer two = core::timer::Timer::singleshotTimer(
            [&second] {
                ++second;
            },
            utils::Timeval(0.01));
        core::timer::Timer three = core::timer::Timer::singleshotTimer(
            [&cancelled] {
                ++cancelled;
            },
            utils::Timeval(0.01));
        freezeClock = false;
        // Restart and cancellation must identify an individual timer even when
        // all three initially share a deadline.
        two.restart();
        three.cancel();
        core::timer::Timer::singleshotTimer(
            [] {
                core::SNodeC::stop();
            },
            utils::Timeval(0.05));
        const int started = core::SNodeC::start(utils::Timeval(1));
        testResult.expectEqual(0, started, "event loop exits normally");
        testResult.expectEqual(1, first, "first equal-deadline timer remains registered after another restarts");
        testResult.expectEqual(1, second, "second equal-deadline timer fires once");
        testResult.expectEqual(0, cancelled, "cancellation affects only its timer");
        core::SNodeC::free();
        result = testResult.processResult();
    }
    return result;
}
