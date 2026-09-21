/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "core/socket/stream/ClientFlowController.h"

#include "core/eventreceiver/ConnectEventReceiver.h"
#include "core/socket/stream/FlowController.hpp"
#include "core/timer/Timer.h"
#include "log/SemanticLogger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    ClientFlowController::ClientFlowController(const std::string& instanceName)
        : FlowController(instanceName, logger::LogRole::Client)
        , onFlowReconnectCallback([](ClientFlowController*) {
        }) {
    }

    void ClientFlowController::stopReconnect() {
        reconnectEnabled = false;
        cancelReconnectTimer();
    }

    bool ClientFlowController::isReconnectEnabled() const {
        return reconnectEnabled;
    }

    std::uint64_t ClientFlowController::getReconnectCount() const noexcept {
        return reconnectCount;
    }

    ClientFlowController* ClientFlowController::setOnFlowReconnect(const std::function<void(ClientFlowController*)>& callback) {
        const std::function<void(ClientFlowController*)> oldCallback = onFlowReconnectCallback;
        onFlowReconnectCallback = [oldCallback, callback](ClientFlowController* flowController) {
            oldCallback(flowController);
            callback(flowController);
        };

        return this;
    }

    bool ClientFlowController::dispatchReconnect() {
        if (!reconnectTimer) {
            return false;
        }
        reconnectTimer.reset();
        log().debug("reconnect dispatched");
        ++reconnectCount;
        onFlowReconnectCallback(this);
        return !isTerminated() && reconnectEnabled;
    }

    void ClientFlowController::observeConnectEventReceiver(core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
        if (connectEventReceiver != nullptr) {
            if (connectEventReceiver->isEnabled()) {
                if (isTerminated()) {
                    connectEventReceiver->stopConnect();
                } else {
                    connectEventReceivers.insert(connectEventReceiver);
                }
            } else {
                connectEventReceivers.erase(connectEventReceiver);
            }
        }
    }

    void ClientFlowController::armReconnectTimer(double timeoutSeconds, const std::function<void()>& dispatcher) {
        if (reconnectEnabled) {
            log().debug("reconnect scheduled");
            reconnectTimer = std::make_unique<core::timer::Timer>(core::timer::Timer::singleshotTimer(dispatcher, timeoutSeconds));
        }
    }

    void ClientFlowController::terminateAsyncSubFlow() {
        stopReconnect();
        stopRetry();

        for (core::eventreceiver::ConnectEventReceiver* connectEventReceiver : connectEventReceivers) {
            if (connectEventReceiver != nullptr) {
                connectEventReceiver->stopConnect();
            }
        }

        connectEventReceivers.clear();
    }

    void ClientFlowController::cancelReconnectTimer() {
        if (reconnectTimer) {
            log().debug("reconnect cancelled");
            reconnectTimer->cancel();
            reconnectTimer.reset();
        }
    }

    template class FlowController<ClientFlowController>;

} // namespace core::socket::stream
