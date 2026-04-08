/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "core/socket/stream/FlowController.h"

namespace core::socket::stream {

    FlowController::FlowController(net::config::ConfigInstance* configInstance)
        : observedConfigInstance(configInstance)
        , id(detail::flowControllerIdCounter.fetch_add(1, std::memory_order_relaxed))
        , onDestroy([](FlowController*) {
        })
        , onFlowRetryCallback([](FlowController*) {
        })
        , onFlowTerminatedCallback([](FlowController*) {
        })
        , onFlowStartedCallback([](FlowController*) {
        }) {
    }

    FlowController::~FlowController() {
        onDestroy(self());
    }

    std::string FlowController::getInstanceName() const {
        return observedConfigInstance->getInstanceName();
    }

    uint64_t FlowController::getId() const {
        return id;
    }

    void FlowController::startFlow(const std::function<void()>& callback) {
        std::weak_ptr<FlowController> selfWeak = shared_from_this();
        core::EventReceiver::atNextTick([selfWeak, callback]() {
            if (const std::shared_ptr<FlowController> selfLocked = selfWeak.lock(); selfLocked != nullptr && !selfLocked->isTerminated()) {
                selfLocked->reportFlowStarted();
                callback();
            }
        });
    }

    bool FlowController::terminateFlow() {
        bool success = false;

        if (!terminated) {
            terminated = true;
            retryEnabled = false;
            cancelRetryTimer();

            terminateAsyncSubFlow();
            notifyFlowTerminated();

            success = true;
        }

        return success;
    }

    bool FlowController::isTerminated() const {
        return terminated;
    }

    void FlowController::stopRetry() {
        retryEnabled = false;
        cancelRetryTimer();
    }

    bool FlowController::isRetryEnabled() const {
        return retryEnabled;
    }

    FlowController* FlowController::setOnDestroy(const std::function<void(FlowController*)>& onDestroySet) {
        observedConfigInstance->setOnDestroy([this, onDestroySet](net::config::ConfigInstance*) {
            onDestroySet(self());
        });

        return self();
    }

    FlowController* FlowController::onFlowRetry(const std::function<void(FlowController*)>& callback) {
        const std::function<void(FlowController*)> oldCallback = onFlowRetryCallback;
        onFlowRetryCallback = [oldCallback, callback](FlowController* flowController) {
            oldCallback(flowController);
            callback(flowController);
        };

        return self();
    }

    FlowController* FlowController::onFlowCompleted(const std::function<void(const std::string&)>& callback) {
        observedConfigInstance->setOnDestroy(
            [instanceName = observedConfigInstance->getInstanceName(), callback](net::config::ConfigInstance*) {
                callback(instanceName);
            });

        return self();
    }

    FlowController* FlowController::onFlowCompleted(const std::function<void(uint64_t)>& callback) {
        observedConfigInstance->setOnDestroy([id = getId(), callback](net::config::ConfigInstance*) {
            callback(id);
        });

        return self();
    }

    FlowController* FlowController::onFlowTerminated(const std::function<void(FlowController*)>& callback) {
        const std::function<void(FlowController*)> oldCallback = onFlowTerminatedCallback;
        onFlowTerminatedCallback = [oldCallback, callback](FlowController* flowController) {
            oldCallback(flowController);
            callback(flowController);
        };

        return self();
    }

    FlowController* FlowController::onFlowStarted(const std::function<void(FlowController*)>& callback) {
        const std::function<void(FlowController*)> oldCallback = onFlowStartedCallback;
        onFlowStartedCallback = [oldCallback, callback](FlowController* flowController) {
            oldCallback(flowController);
            callback(flowController);
        };

        return self();
    }

    void FlowController::reportFlowRetry() {
        onFlowRetryCallback(self());
    }

    void FlowController::reportFlowStarted() {
        onFlowStartedCallback(self());
    }

    void FlowController::armRetryTimer(double timeoutSeconds, const std::function<void()>& dispatcher) {
        if (retryEnabled) {
            retryTimer = std::make_unique<core::timer::Timer>(core::timer::Timer::singleshotTimer(dispatcher, timeoutSeconds));
        }
    }

    FlowController* FlowController::self() {
        return this;
    }

    void FlowController::cancelRetryTimer() {
        if (retryTimer) {
            retryTimer->cancel();
            retryTimer.reset();
        }
    }

    void FlowController::notifyFlowTerminated() {
        onFlowTerminatedCallback(self());
    }

} // namespace core::socket::stream
