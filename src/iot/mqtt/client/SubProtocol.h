/*
 * SNode.C - a slim toolkit for network communication
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

#ifndef IOT_MQTT_CLIENT_MQTTSUBPROTOCOL_H
#define IOT_MQTT_CLIENT_MQTTSUBPROTOCOL_H

#include "iot/mqtt/SubProtocol.h" // IWYU pragma: export
#include "web/websocket/client/SubProtocol.h"

// IWYU pragma: no_include "iot/mqtt/SubProtocol.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::client {

    using SubProtocol = iot::mqtt::SubProtocol<web::websocket::client::SubProtocol>;

} // namespace iot::mqtt::client

extern template class iot::mqtt::SubProtocol<web::websocket::client::SubProtocol>;

#endif // IOT_MQTT_CLIENT_MQTTSUBPROTOCOL_H
