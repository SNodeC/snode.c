/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "Session.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json.hpp>
#include <utility>

// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    Session::Session(const nlohmann::json& json) {
        fromJson(json);
    }

    nlohmann::json Session::toJson() {
        nlohmann::json json;

        std::vector<nlohmann::json> publishJsonVector;
        for (auto& [packetIdentifier, publish] : publishMap) {
            nlohmann::json publishJson;

            publishJson["packet_identifier"] = packetIdentifier;
            publishJson["topic"] = publish.getTopic();
            publishJson["message"] = publish.getMessage();
            publishJson["dup"] = publish.getDup();
            publishJson["qos"] = publish.getQoS();
            publishJson["retain"] = publish.getRetain();

            publishJsonVector.emplace_back(publishJson);
        }

        if (!publishJsonVector.empty()) {
            json["publish_map"] = publishJsonVector;
        }
        if (!pubrelPacketIdentifierSet.empty()) {
            json["pubrel_packetidentifier_set"] = pubrelPacketIdentifierSet;
        }
        if (!publishPacketIdentifierSet.empty()) {
            json["publish_packetidentifier_set"] = publishPacketIdentifierSet;
        }

        return json;
    }

    void Session::fromJson(const nlohmann::json& json) {
        if (!json.empty()) {
            if (json.contains("pubrel_packetidentifier_set")) {
                pubrelPacketIdentifierSet = static_cast<std::set<uint16_t>>(json["pubrel_packetidentifier_set"]);
            }
            if (json.contains("publish_packetidentifier_set")) {
                publishPacketIdentifierSet = static_cast<std::set<uint16_t>>(json["publish_packetidentifier_set"]);
            }

            if (json.contains("publish_map")) {
                for (auto publishJson : json["publish_map"]) {
                    iot::mqtt::packets::Publish publish(publishJson["packet_identifier"],
                                                        publishJson["topic"],
                                                        publishJson["message"],
                                                        publishJson["qos"],
                                                        publishJson["dup"],
                                                        publishJson["retain"]);
                    publishMap[publishJson["packet_identifier"]] = publish;
                }
            }
        }
    }

} // namespace iot::mqtt
