/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "Session.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json.hpp>
#include <vector>

// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {

    Session::Session(const nlohmann::json& json) {
        fromJson(json);
    }

    nlohmann::json Session::toJson() const {
        nlohmann::json json;

        std::vector<nlohmann::json> publishJsonVector;
        for (const auto& [packetIdentifier, publish] : outgoingPublishMap) {
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

        // QoS 2 inbound state (receiver side)
        std::vector<nlohmann::json> incomingPublishJsonVector;
        for (const auto& [packetIdentifier, publish] : incomingPublishMap) {
            nlohmann::json publishJson;

            publishJson["packet_identifier"] = packetIdentifier;
            publishJson["topic"] = publish.getTopic();
            publishJson["message"] = publish.getMessage();
            publishJson["dup"] = publish.getDup();
            publishJson["qos"] = publish.getQoS();
            publishJson["retain"] = publish.getRetain();

            incomingPublishJsonVector.emplace_back(publishJson);
        }

        if (!incomingPublishJsonVector.empty()) {
            json["incoming_publish_map"] = incomingPublishJsonVector;
        }
        if (!pubcompPacketIdentifierSet.empty()) {
            json["pubcomp_packetidentifier_set"] = pubcompPacketIdentifierSet;
        }

        return json;
    }

    void Session::fromJson(const nlohmann::json& json) {
        if (json.contains("pubrel_packetidentifier_set")) {
            pubrelPacketIdentifierSet = static_cast<std::set<uint16_t>>(json["pubrel_packetidentifier_set"]);
        }
        if (json.contains("publish_map")) {
            for (const auto& publishJson : json["publish_map"]) {
                if (publishJson.contains("packet_identifier")) {
                    outgoingPublishMap.emplace(publishJson["packet_identifier"],
                                               iot::mqtt::packets::Publish(publishJson["packet_identifier"],
                                                                           publishJson["topic"],
                                                                           publishJson["message"],
                                                                           publishJson["qos"],
                                                                           publishJson["dup"],
                                                                           publishJson["retain"]));
                }
            }
        }

        // QoS 2 inbound state (receiver side)
        if (json.contains("pubcomp_packetidentifier_set")) {
            pubcompPacketIdentifierSet = static_cast<std::set<uint16_t>>(json["pubcomp_packetidentifier_set"]);
        }

        if (json.contains("incoming_publish_map")) {
            for (const auto& publishJson : json["incoming_publish_map"]) {
                if (publishJson.contains("packet_identifier")) {
                    incomingPublishMap.emplace(publishJson["packet_identifier"],
                                               iot::mqtt::packets::Publish(publishJson["packet_identifier"],
                                                                           publishJson["topic"],
                                                                           publishJson["message"],
                                                                           publishJson["qos"],
                                                                           publishJson["dup"],
                                                                           publishJson["retain"]));
                }
            }
        }
    }

    void Session::clear() {
        outgoingPublishMap.clear();
        pubrelPacketIdentifierSet.clear();
        incomingPublishMap.clear();
        pubcompPacketIdentifierSet.clear();
    }

} // namespace iot::mqtt
