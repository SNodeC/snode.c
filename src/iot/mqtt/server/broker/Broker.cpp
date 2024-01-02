/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "iot/mqtt/server/broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <initializer_list>
#include <nlohmann/json.hpp>
#include <vector>

// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt::server::broker {

    Broker::Broker(uint8_t maxQoS)
        : sessionStoreFileName((getenv("MQTT_SESSION_STORE") != nullptr) ? getenv("MQTT_SESSION_STORE") : "") // NOLINT
        , maxQoS(maxQoS)
        , subscribtionTree(this)
        , retainTree(this) {
        if (!sessionStoreFileName.empty()) {
            std::ifstream sessionStoreFile(sessionStoreFileName);

            if (sessionStoreFile.is_open()) {
                try {
                    nlohmann::json sessionStoreJson;

                    sessionStoreFile >> sessionStoreJson;

                    for (const auto& [clientId, sessionJson] : sessionStoreJson["session_store"].items()) {
                        sessionStore[clientId].fromJson(sessionJson);
                    }
                    retainTree.fromJson(sessionStoreJson["retain_tree"]);
                    subscribtionTree.fromJson(sessionStoreJson["subscribtion_tree"]);

                    LOG(DEBUG) << "MQTT Broker: Persistent session data loaded successful";
                } catch (const nlohmann::json::exception&) {
                    LOG(DEBUG) << "MQTT Broker: Starting with empty session: Session store '" << sessionStoreFileName
                               << "' empty or corrupted";

                    sessionStore.clear();
                    retainTree.clear();
                    subscribtionTree.clear();
                }

                sessionStoreFile.close();
                std::remove(sessionStoreFileName.data()); // NOLINT

                LOG(INFO) << "MQTT Broker: Restoring saved session done";
            } else {
                PLOG(INFO) << "MQTT Broker: Could not read session store '" << sessionStoreFileName << "'";
            }
        } else {
            LOG(INFO) << "MQTT Broker: Session not reloaded: Session store filename empty";
        }
    }

    Broker::~Broker() {
        if (!sessionStoreFileName.empty()) {
            nlohmann::json sessionStoreJson;

            for (auto& [clientId, session] : sessionStore) {
                sessionStoreJson["session_store"][clientId] = session.toJson();
            }
            sessionStoreJson["retain_tree"] = retainTree.toJson();
            sessionStoreJson["subscribtion_tree"] = subscribtionTree.toJson();

            if (sessionStoreJson["session_store"].empty()) {
                sessionStoreJson.erase("session_store");
            }
            if (sessionStoreJson["retain_tree"].empty()) {
                sessionStoreJson.erase("retain_tree");
            }
            if (sessionStoreJson["subscribtion_tree"].empty()) {
                sessionStoreJson.erase("subscribtion_tree");
            }

            std::ofstream sessionStoreFile(sessionStoreFileName);

            if (sessionStoreFile.is_open()) {
                if (!sessionStoreJson.empty()) {
                    sessionStoreFile << sessionStoreJson;
                }

                sessionStoreFile.close();

                LOG(DEBUG) << "MQTT Broker: Session store written '" << sessionStoreFileName << "'";
            } else {
                PLOG(DEBUG) << "MQTT Broker: Could not write session store '" << sessionStoreFileName << "'";
            }
        } else {
            LOG(INFO) << "MQTT Broker: Session not saved: Session store filename empty";
        }
    }

    std::shared_ptr<Broker> Broker::instance(uint8_t maxQoS) {
        static const std::shared_ptr<Broker> broker = std::make_shared<Broker>(maxQoS);

        return broker;
    }

    void Broker::appear(const std::string& clientId, const std::string& topic, uint8_t qoS) {
        retainTree.appear(clientId, topic, qoS);
    }

    void Broker::unsubscribe(const std::string& clientId) {
        subscribtionTree.unsubscribe(clientId);
    }

    void
    Broker::publish(const std::string& originClientId, const std::string& topic, const std::string& message, uint8_t qoS, bool retain) {
        subscribtionTree.publish(Message(originClientId, topic, message, qoS, retain));

        if (retain) {
            retainTree.retain(Message(originClientId, topic, message, qoS, retain));
        }
    }

    uint8_t Broker::subscribe(const std::string& clientId, const std::string& topic, uint8_t qoS) {
        qoS = std::min(maxQoS, qoS);
        uint8_t returnCode = 0;

        if (subscribtionTree.subscribe(topic, clientId, qoS)) {
            retainTree.appear(clientId, topic, qoS);

            returnCode = SUBSCRIBTION_SUCCESS | qoS;
        } else {
            returnCode = SUBSCRIBTION_FAILURE;
        }

        return returnCode;
    }

    void Broker::unsubscribe(const std::string& clientId, const std::string& topic) {
        subscribtionTree.unsubscribe(topic, clientId);
    }

    bool Broker::hasSession(const std::string& clientId) {
        return sessionStore.contains(clientId);
    }

    bool Broker::hasActiveSession(const std::string& clientId) {
        return hasSession(clientId) && sessionStore[clientId].isActive();
    }

    bool Broker::hasRetainedSession(const std::string& clientId) {
        return hasSession(clientId) && !sessionStore[clientId].isActive();
    }

    bool Broker::isActiveSession(const std::string& clientId, const iot::mqtt::server::Mqtt* mqtt) {
        return hasSession(clientId) && sessionStore[clientId].isOwnedBy(mqtt);
    }

    Session* Broker::newSession(const std::string& clientId, iot::mqtt::server::Mqtt* mqtt) {
        sessionStore[clientId] = iot::mqtt::server::broker::Session(mqtt);

        return &sessionStore[clientId];
    }

    Session* Broker::renewSession(const std::string& clientId, iot::mqtt::server::Mqtt* mqtt) {
        return sessionStore[clientId].renew(mqtt);
    }

    void Broker::restartSession(const std::string& clientId) {
        LOG(DEBUG) << "MQTT Broker:   Retained: Send Publish: " << clientId;
        subscribtionTree.appear(clientId);

        LOG(DEBUG) << "MQTT Broker:   Queued: Send Publish: " << clientId;
        sessionStore[clientId].publishQueued();
    }

    void Broker::retainSession(const std::string& clientId) {
        sessionStore[clientId].retain();
    }

    void Broker::deleteSession(const std::string& clientId) {
        subscribtionTree.unsubscribe(clientId);
        sessionStore.erase(clientId);
    }

    void Broker::sendPublish(const std::string& clientId, Message& message, uint8_t qoS, bool retain) {
        LOG(DEBUG) << "MQTT Broker: Send Publish: " << clientId;
        sessionStore[clientId].sendPublish(message, qoS, retain);
    }

} // namespace iot::mqtt::server::broker
