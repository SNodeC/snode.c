#include "log/Logger.h"
#include "net/SNodeC.h"
#include "net/socket/ip/tcp/ipv4/legacy/SocketServer.h"

#include <cstddef>
#include <endian.h>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace net;

using namespace net::socket::ip::tcp::ipv4::legacy;

class WSMessage {
public:
    WSMessage(unsigned char opCode, const std::vector<char>& message)
        : message(message)
        , opCode(opCode) {
    }
    std::vector<char> message;
    unsigned char opCode = 0;
};

class WebSocketReceiver {
public:
    void receive(const char* junk, std::size_t junkLen) {
        uint64_t consumed = 0;
        bool parsingError = false;

        while (consumed < junkLen && !parsingError) {
            switch (parserState) {
                case ParserState::BEGIN:
                    parserState = ParserState::OPCODE;
                    begin();
                    [[fallthrough]];
                case ParserState::OPCODE:
                    consumed += readOpcode(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::LENGTH:
                    consumed += readLength(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::ELENGTH:
                    consumed += readELength(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::MASKINGKEY:
                    consumed += readMaskingKey(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::PAYLOAD:
                    consumed += readPayload(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::ERROR:
                    parsingError = true;
                    reset();
                    break;
            };
        }
    }

protected:
    void begin() {
    }

    uint64_t readOpcode(const char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;
        if (junkLen > 0) {
            consumed++;

            uint8_t opCodeByte = static_cast<uint8_t>(junk[0]);

            fin = opCodeByte & 0b10000000;
            opCode = (opCodeByte & 0b00001111) == 0 ? opCode : opCodeByte & 0b00001111;

            parserState = ParserState::LENGTH;
        }

        return consumed;
    }

    uint64_t readLength(const char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        if (junkLen > 0) {
            uint8_t lengthByte = static_cast<uint8_t>(junk[0]);

            masked = lengthByte & 0b10000000;
            length = lengthByte & 0b01111111;

            if (length > 125) {
                switch (length) {
                    case 126:
                        elengthNumBytes = 2;
                        break;
                    case 127:
                        elengthNumBytes = 8;
                        break;
                }
                parserState = ParserState::ELENGTH;
                length = 0;
            } else {
                parserState = ParserState::MASKINGKEY;
            }
            consumed++;
        }

        return consumed;
    }

    uint64_t readELength(const char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        elengthNumBytesLeft = (elengthNumBytesLeft == 0) ? elengthNumBytes : elengthNumBytesLeft;

        while (consumed < junkLen && elengthNumBytesLeft > 0) {
            length |= static_cast<uint64_t>(junk[consumed]) << (elengthNumBytes - elengthNumBytesLeft) * 8;

            consumed++;
            elengthNumBytesLeft--;
        }

        if (elengthNumBytesLeft == 0) {
            switch (elengthNumBytes) {
                case 2: {
                    uint16_t length16 = static_cast<uint16_t>(length);
                    length = be16toh(length16);
                } break;
                case 8:
                    length = be64toh(length);
                    break;
            }
            parserState = ParserState::MASKINGKEY;
        }

        return consumed;
    }

    uint64_t readMaskingKey(const char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        if (masked) {
            maskingKeyNumBytesLeft = (maskingKeyNumBytesLeft == 0) ? maskingKeyNumBytes : maskingKeyNumBytesLeft;

            while (consumed < junkLen && maskingKeyNumBytesLeft > 0) {
                maskingKey |= static_cast<uint32_t>(junk[consumed]) << (maskingKeyNumBytes - maskingKeyNumBytesLeft) * 8;
                consumed++;
                maskingKeyNumBytesLeft--;
            }

            if (maskingKeyNumBytesLeft == 0) {
                maskingKey = be32toh(maskingKey);
                parserState = ParserState::PAYLOAD;
            }
        } else {
            parserState = ParserState::PAYLOAD;
        }

        return consumed;
    }

    uint64_t readPayload(const char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        if (junkLen > 0 && length - payloadRead > 0) {
            uint64_t numBytesToRead = (junkLen <= length - payloadRead) ? junkLen : length - payloadRead;
            messageData.insert(messageData.end(), junk, junk + numBytesToRead);
            payloadRead += numBytesToRead;
            consumed = numBytesToRead;
        }

        reset();

        return consumed;
    }

    void printFrame() {
        VLOG(0) << "--------- Frame ---------";
        VLOG(0) << "FIN: " << fin;
        VLOG(0) << "OPCODE: " << (int) opCode;
        VLOG(0) << "LENGTH: " << length;
        VLOG(0) << "MASK: " << masked;
        if (masked) {
            VLOG(0) << "MASKKEY: "
                    << "0x" << std::setfill('0') << std::setw(8) << std::hex << maskingKey;
        }
        std::string s(messageData.data(), messageData.size());
        VLOG(0) << "Data: " << s; // UTF-8 encoded
    }

    void printMessage(const WSMessage& wSMessage) {
        VLOG(0) << "--------- Message ---------";
        VLOG(0) << "OPCode: " << (int) wSMessage.opCode;
        std::string s(wSMessage.message.data(), wSMessage.message.size());
        VLOG(0) << "Data: " << s; // UTF-8 encoded
    }

    void reset() {
        if (payloadRead == length) {
            printFrame();
            payloadRead = 0;
            if (fin) {
                // Message ready
                WSMessage wSMessage(opCode, messageData);
                printMessage(wSMessage);

                messageData.clear();
                opCode = 0;
            }
            parserState = ParserState::BEGIN;
            fin = false;
            masked = false;
            length = 0;
            elengthNumBytes = 0;
            elengthNumBytesLeft = 0;
            maskingKey = 0;
            maskingKeyNumBytes = 4;
            maskingKeyNumBytesLeft = 0;
            payloadRead = 0;
        }
    }

    void onMessageData() {
    }

    // uint64_t htobe64(uint64_t host_64bits);
    // uint64_t be64toh(uint64_t big_endian_64bits);
    // uint16_t htobe16(uint16_t host_16bits);
    // uint16_t be16toh(uint16_t big_endian_16bits);

protected:
    // Parser state
    enum struct ParserState { BEGIN, OPCODE, LENGTH, ELENGTH, MASKINGKEY, PAYLOAD, ERROR } parserState = ParserState::BEGIN;

    bool fin = false;
    bool masked = false;
    uint8_t opCode = 0;
    uint64_t length = 0;
    uint32_t maskingKey = 0;
    std::vector<char> messageData;

    uint8_t elengthNumBytes = 0;
    uint8_t elengthNumBytesLeft = 0;
    uint8_t maskingKeyNumBytes = 4;
    uint8_t maskingKeyNumBytesLeft = 0;
    uint64_t payloadRead = 0;
};

#define WSPAYLOADLENGTH 4

class WebSocketSender {
public:
    WebSocketReceiver webSocketReceiver;

public:
    void send(uint8_t opCode, const char* message, std::size_t messageLength) {
        std::size_t messageOffset = 0;

        while (messageLength - messageOffset > 0) {
            std::cout << "-----------------" << std::endl;
            std::size_t sendMessageLength =
                (messageLength - messageOffset <= WSPAYLOADLENGTH) ? messageLength - messageOffset : WSPAYLOADLENGTH;
            bool fin = sendMessageLength == messageLength - messageOffset;
            sendFrame(fin, opCode, 0x01020304, message + messageOffset, sendMessageLength);
            messageOffset += sendMessageLength;
            opCode = 0; // continuation
            reset();
        }
    }

protected:
    void sendFrame(bool fin, uint8_t opCode, uint32_t maskingKey, const char* payload, uint64_t payloadLength) {
        uint64_t frameLength = ((maskingKey > 0) ? 6 : 2) + payloadLength;

        char* frame = nullptr;
        uint64_t length = 0;

        if (payloadLength > 125) {
            if (payloadLength > 0xFFFF) {
                frameLength += 8;
                maskingKeyOffset += 8;
                payloadOffset += 8;
                frame = new char[frameLength];
                *reinterpret_cast<uint64_t*>(frame + eLengthOffset) = htobe64(*reinterpret_cast<uint64_t*>(&payloadLength));
                length = 127;
            } else {
                frameLength += 2;
                maskingKeyOffset += 2;
                payloadOffset += 2;
                frame = new char[frameLength];
                *reinterpret_cast<uint16_t*>(frame + eLengthOffset) = htobe16(*reinterpret_cast<uint16_t*>(&payloadLength));
                length = 126;
            }
        } else {
            frame = new char[frameLength];
            length = payloadLength;
        }

        if (maskingKey > 0) {
            *reinterpret_cast<uint32_t*>(frame + maskingKeyOffset) = htobe32(maskingKey);
            payloadOffset += 4;
        }

        *reinterpret_cast<uint8_t*>(frame + opCodeOffset) = static_cast<uint8_t>((fin ? 0b10000000 : 0) | opCode);
        *reinterpret_cast<uint8_t*>(frame + lengthOffset) = static_cast<uint8_t>(((maskingKey > 0) ? 0b10000000 : 0) | length);

        memcpy(frame + payloadOffset, payload, static_cast<std::size_t>(payloadLength));

        for (std::size_t i = 0; i < frameLength; i++) {
            std::cout << std::hex << (unsigned int) (unsigned char) frame[i] << " ";

            if ((i + 1) % 4 == 0) {
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;

        //        wSParser.receive(frame, length);

        for (unsigned long i = 0; i < frameLength; i++) {
            webSocketReceiver.receive(frame + i, 1);
        }

        delete[] frame;
    }

    void reset() {
        opCodeOffset = 0;
        lengthOffset = 1;
        eLengthOffset = 2;
        maskingKeyOffset = 2;
        payloadOffset = 2;
    }

    uint8_t opCodeOffset = 0;
    uint8_t lengthOffset = 1;
    uint8_t eLengthOffset = 2;
    uint8_t maskingKeyOffset = 2;
    uint8_t payloadOffset = 2;
};

int main(int argc, char* argv[]) {
    SNodeC::init(argc, argv);

    WebSocketSender webSocketSender;

    webSocketSender.send(1, "Hallo Du", std::string("Hallo Du").length());

    SocketServer webSocketParser(
        [](const SocketServer::SocketAddress& localAddress,
           const SocketServer::SocketAddress& remoteAddress) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tServer: " + localAddress.toString();
            VLOG(0) << "\tClient: " + remoteAddress.toString();
        },
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection) -> void { // onConnected
            VLOG(0) << "OnConnected";
        },
        [](SocketServer::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";

            VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().toString();
        },
        [](SocketServer::SocketConnection* socketConnection, const char* junk, std::size_t junkLen) -> void { // onRead
            std::string data(junk, junkLen);
            VLOG(0) << "Data to reflect: " << data;
            socketConnection->enqueue(data);
        },
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection, int errnum) -> void { // onReadError
            PLOG(ERROR) << "OnReadError: " << errnum;
        },
        []([[maybe_unused]] SocketServer::SocketConnection* socketConnection, int errnum) -> void { // onWriteError
            PLOG(ERROR) << "OnWriteError: " << errnum;
        });

    webSocketParser.listen(SocketServer::SocketAddress(8080), 5, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8080 for legacy connections";
        }
    });

    return SNodeC::start();
}
