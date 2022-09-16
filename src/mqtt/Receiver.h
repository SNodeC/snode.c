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

#ifndef MQTT_RECEIVER_H
#define MQTT_RECEIVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt {

    class Receiver {
    public:
        Receiver();

        std::size_t receive();

    private:
        enum struct ParserState { BEGIN, FIXEDHEADER, VARIABLEHEADER, PAYLOAD, ERROR } parserState = ParserState::BEGIN;
        /*
         *      Type        Nr | Direction        | Action                   Package Identifier | VH | PL | Flags (4Bits)
         *      ==============================================================================================
                Reserved     0 | Forbidden        | Reserved                                  N |  N |  N | 0x00
                CONNECT      1 | Client to Server | Connection request                        N |  Y |  N | 0x00
                CONNACK      2 | Server to Client | Connect acknowledgment                    N |  Y |  N | 0x00
                PUBLISH      3 | Both directions  | Publish message               if QoS > 0: Y |  Y |  Y | DUP(1)
                                                                                                |           QoS1(2)
                                                                                                            RETAIN(1)
                PUBACK       4 | Both directions  | Publish acknowledge                       Y |  Y |  N | 0x00
                PUBREC       5 | Both directions  | Publish received (QoS 2 delivery part 1)  Y |  Y |  N | 0x00
                PUBREL       6 | Both directions  | Publish release (QoS 2 delivery part 2)   Y |  Y |  N | 0x02
                PUBCOMP      7 | Both directions  | Publish complete (QoS 2 delivery part 3)  Y |  Y |  N | 0x00
                SUBSCRIBE    8 | Client to Server | Subscribe request                         Y |  Y |  Y | 0x02
                SUBACK       9 | Server to Client | Subscribe acknowledgment                  Y |  Y |  Y | 0x00
                UNSUBSCRIBE 10 | Client to Server | Unsubscribe request                       Y |  Y |  Y | 0x02
                UNSUBACK    11 | Server to Client | Unsubscribe acknowledgment                Y |  Y |  N | 0x00
                PINGREQ     12 | Client to Server | PING request                              N |  N |  N | 0x00
                PINGRESP    13 | Server to Client | PING response                             N |  N |  N | 0x00
                DISCONNECT  14 | Both directions  | Disconnect notification                   N |  N |  N | 0x00
                AUTH        15 | Both directions  | Authentication exchange                   N |  Y |  Y | 0x00

        // ***********************++

           Int_1  uint8_t                                 one byte integer   Reader/Writer
           Int_2  uint16_t                                two byte integer   Reader/Writer
           Int_4  uint32_t                                four byte integer  Reader/Writer
           Int_V  uint32_t                                variable integer   Reader/Writer
           String std::string                             string             Reader/Writer
           Binary std::vector                             binary data        Reader/Writer (two byte integer length + data)
           StringPair std::pair<std::string, std::string> string-pair        Reader/Writer (two String)

        // *************************

           FixedHeader     | Int_1 (ControlPacketType 4Bits | Flags 4Bits)      | Int_V (RemainingLength)          |
           Variable Header | Int_2 (Packet Identifier) | Int_V (Property Length | Int_V (Property Identifier)      | * (Property Value)
           Payload




        */
    };

} // namespace mqtt

#endif // MQTT_RECEIVER_H
