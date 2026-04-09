/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "core/socket/stream/ClientFlowController.h"

#include "core/eventreceiver/ConnectEventReceiver.h"
#include "core/socket/stream/FlowController.hpp"
#include "core/timer/Timer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    ClientFlowController::ClientFlowController(net::config::ConfigInstance* configInstance)
        : FlowController(configInstance)
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

    ClientFlowController* ClientFlowController::setOnFlowReconnect(const std::function<void(ClientFlowController*)>& callback) {
        const std::function<void(ClientFlowController*)> oldCallback = onFlowReconnectCallback;
        onFlowReconnectCallback = [oldCallback, callback](ClientFlowController* flowController) {
            oldCallback(flowController);
            callback(flowController);
        };

        return this;
    }

    void ClientFlowController::reportFlowReconnect() {
        onFlowReconnectCallback(this);
    }

    void ClientFlowController::observeConnectEventReceiver(core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
        if (connectEventReceiver != nullptr) {
            if (connectEventReceiver->isEnabled()) {
                connectEventReceivers.insert(connectEventReceiver);
            } else {
                connectEventReceivers.erase(connectEventReceiver);
            }
        }
    }

    void ClientFlowController::armReconnectTimer(double timeoutSeconds, const std::function<void()>& dispatcher) {
        if (reconnectEnabled) {
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
            reconnectTimer->cancel();
            reconnectTimer.reset();
        }
    }

    template class FlowController<ClientFlowController>;

} // namespace core::socket::stream
