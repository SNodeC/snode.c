/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "support/DescriptorRegistrationFailure.h"

#include "core/DescriptorEventPublisher.h"
#include "core/DescriptorEventReceiver.h"
#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"
#include "core/SNodeC.h"
#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/pipe/Pipe.h"
#include "support/TestResult.h"

#include <cerrno>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unistd.h>

namespace {
    class RejectingPublisher final : public core::DescriptorEventPublisher {
    public:
        explicit RejectingPublisher(bool throwOnAdd)
            : core::DescriptorEventPublisher("rejecting test publisher")
            , throwOnAdd(throwOnAdd) {
        }

        int deleteCount = 0;

    private:
        bool muxAdd([[maybe_unused]] core::DescriptorEventReceiver* receiver) override {
            if (throwOnAdd) {
                throw std::runtime_error("injected muxAdd failure");
            }
            return false;
        }

        void muxDel([[maybe_unused]] int fd) override {
            ++deleteCount;
        }

        void muxOn([[maybe_unused]] core::DescriptorEventReceiver* receiver) override {
        }

        void muxOff([[maybe_unused]] core::DescriptorEventReceiver* receiver) override {
        }

        void spanActiveEvents() override {
        }

        bool throwOnAdd;
    };

    class PublisherProbe final : public core::DescriptorEventReceiver {
    public:
        explicit PublisherProbe(core::DescriptorEventPublisher& publisher)
            : core::DescriptorEventReceiver("publisher registration probe", publisher, TIMEOUT::DISABLE) {
        }

        ~PublisherProbe() override = default;

        bool registerFd(int fd) {
            return enable(fd);
        }

    private:
        void dispatchEvent() override {
        }

        void timeoutEvent() override {
        }

        void signalEvent([[maybe_unused]] int signalNumber) override {
        }

        void unobservedEvent() override {
        }
    };

    class RealPublisherProbe final : public core::eventreceiver::ReadEventReceiver {
    public:
        RealPublisherProbe()
            : ReadEventReceiver("real descriptor registration probe", TIMEOUT::DISABLE) {
        }

        ~RealPublisherProbe() override = default;

        bool registerFd(int fd) {
            return enable(fd);
        }

        void unregisterFd() {
            disable();
        }

    private:
        void readEvent() override {
        }

        void unobservedEvent() override {
        }
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("DescriptorRegistrationFailureTest");
        return tests::support::cTestSkipReturnCode;
    }

    RejectingPublisher rejectingPublisher(false);
    PublisherProbe rejectedProbe(rejectingPublisher);
    testResult.expectTrue(!rejectedProbe.registerFd(42), "muxAdd rejection propagates through DescriptorEventReceiver::enable");
    testResult.expectTrue(!rejectedProbe.isEnabled() && rejectedProbe.getRegisteredFd() < 0,
                          "a rejected receiver remains disabled and forgets the attempted descriptor");
    testResult.expectEqual(
        0, rejectingPublisher.getObservedEventReceiverCount(), "publisher bookkeeping is rolled back after muxAdd rejection");
    testResult.expectEqual(0, rejectingPublisher.deleteCount, "failed registration does not issue a muxDel for an unregistered descriptor");

    RejectingPublisher throwingPublisher(true);
    PublisherProbe throwingProbe(throwingPublisher);
    testResult.expectTrue(!throwingProbe.registerFd(43), "muxAdd exceptions are converted into observable registration failure");
    testResult.expectEqual(
        0, throwingPublisher.getObservedEventReceiverCount(), "publisher bookkeeping is rolled back after a muxAdd exception");

    core::SNodeC::init(argc, argv);
    core::DescriptorEventPublisher& readPublisher =
        core::EventLoop::instance().getEventMultiplexer().getDescriptorEventPublisher(core::EventMultiplexer::DISP_TYPE::RD);
    const int registrationsBefore = readPublisher.getObservedEventReceiverCount();

    core::pipe::Pipe closedDescriptorPipe;
    const int closedFd = closedDescriptorPipe.releaseReadFd();
    ::close(closedFd);
    RealPublisherProbe closedDescriptorProbe;
    testResult.expectTrue(!closedDescriptorProbe.registerFd(closedFd), "the configured multiplexer rejects a closed positive descriptor");
    testResult.expectEqual(registrationsBefore,
                           readPublisher.getObservedEventReceiverCount(),
                           "real multiplexer rejection leaves publisher bookkeeping unchanged");

    core::pipe::Pipe injectedFailurePipe;
    RealPublisherProbe injectedFailureProbe;
    core::test::failDescriptorRegistrationAfter(0);
    testResult.expectTrue(!injectedFailureProbe.registerFd(injectedFailurePipe.getReadFd()),
                          "the one-shot test injection produces an observable registration failure");
    core::test::clearDescriptorRegistrationFailure();
    testResult.expectEqual(registrationsBefore,
                           readPublisher.getObservedEventReceiverCount(),
                           "injected registration failure also rolls back publisher bookkeeping");

    core::pipe::Pipe failedTransferPipe;
    const int retainedReadFd = failedTransferPipe.getReadFd();
    bool transferFailureObserved = false;
    core::test::failDescriptorRegistrationAfter(0);
    try {
        static_cast<void>(failedTransferPipe.releaseReadAsSink());
    } catch (const std::system_error& exception) {
        transferFailureObserved = exception.code().value() == EIO;
    }
    core::test::clearDescriptorRegistrationFailure();
    testResult.expectTrue(transferFailureObserved, "Pipe exposes receiver-registration failure to its caller");
    testResult.expectTrue(failedTransferPipe.hasReadFd() && failedTransferPipe.getReadFd() == retainedReadFd,
                          "Pipe retains endpoint ownership when receiver registration fails");
    testResult.expectEqual(registrationsBefore,
                           readPublisher.getObservedEventReceiverCount(),
                           "failed Pipe transfer also leaves publisher bookkeeping unchanged");

    core::pipe::Pipe doubleEnablePipe;
    RealPublisherProbe doubleEnableProbe;
    testResult.expectTrue(doubleEnableProbe.registerFd(doubleEnablePipe.getReadFd()), "the first valid descriptor registration succeeds");
    testResult.expectTrue(!doubleEnableProbe.registerFd(doubleEnablePipe.getWriteFd()),
                          "double enable returns false because the supplied descriptor was not registered");
    testResult.expectTrue(doubleEnableProbe.getRegisteredFd() == doubleEnablePipe.getReadFd(),
                          "double enable leaves the original registered descriptor unchanged");
    doubleEnableProbe.unregisterFd();

    core::SNodeC::free();
    return testResult.processResult();
}
