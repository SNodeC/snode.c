#include "log/Logger.h"
#include "net/SNodeC.h"
#include "net/socket/ip/tcp/ipv4/legacy/SocketServer.h"

#include <cstddef>
#include <endian.h>
#include <iomanip>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <vector>

using namespace net;

using namespace net::socket::ip::tcp::ipv4::legacy;

char* base64(const unsigned char* input, int length) {
    const int pl = 4 * ((length + 2) / 3);
    char* output = reinterpret_cast<char*>(calloc(pl + 1, 1)); //+1 for the terminating null that EVP_EncodeBlock adds on
    const int ol = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(output), input, length);
    if (pl != ol) {
        std::cerr << "Whoops, encode predicted " << pl << " but we got " << ol << "\n";
    }
    return output;
}

unsigned char* decode64(const char* input, int length) {
    const int pl = 3 * length / 4;
    unsigned char* output = reinterpret_cast<unsigned char*>(calloc(pl + 1, 1));
    const int ol = EVP_DecodeBlock(output, reinterpret_cast<const unsigned char*>(input), length);
    if (pl != ol) {
        std::cerr << "Whoops, decode predicted " << pl << " but we got " << ol << "\n";
    }
    return output;
}

std::string serverWebSocketKey(const std::string& clientWebSocketKey) {
    std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    std::string serverWebSocketKey(clientWebSocketKey + GUID);
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(serverWebSocketKey.c_str()),
         serverWebSocketKey.length(),
         reinterpret_cast<unsigned char*>(&digest));

    return base64(digest, SHA_DIGEST_LENGTH);
}

union MaskingKeyAsArray {
    uint32_t value;
    char array[4];
};

class WSMessage {
public:
    WSMessage(unsigned char opCode, const std::vector<char>& message)
        : message(message)
        , opCode(opCode) {
    }
    std::vector<char> message;
    unsigned char opCode = 0;

    //   |Opcode  | Meaning                             | Reference |
    //  -+--------+-------------------------------------+-----------|
    //   | 0      | Continuation Frame                  | RFC XXXX  |
    //  -+--------+-------------------------------------+-----------|
    //   | 1      | Text Frame                          | RFC XXXX  |
    //  -+--------+-------------------------------------+-----------|
    //   | 2      | Binary Frame                        | RFC XXXX  |
    //  -+--------+-------------------------------------+-----------|
    //   | 8      | Connection Close Frame              | RFC XXXX  |
    //  -+--------+-------------------------------------+-----------|
    //   | 9      | Ping Frame                          | RFC XXXX  |
    //  -+--------+-------------------------------------+-----------|
    //   | 10     | Pong Frame                          | RFC XXXX  |
    //  -+--------+-------------------------------------+-----------|

    //   | Version Number | Reference                               |
    //  -+----------------+-----------------------------------------+-
    //   | 0              + draft-ietf-hybi-thewebsocketprotocol-00 |
    //  -+----------------+-----------------------------------------+-
    //   | 1              + draft-ietf-hybi-thewebsocketprotocol-01 |
    //  -+----------------+-----------------------------------------+-
    //   | 2              + draft-ietf-hybi-thewebsocketprotocol-02 |
    //  -+----------------+-----------------------------------------+-
    //   | 3              + draft-ietf-hybi-thewebsocketprotocol-03 |
    //  -+----------------+-----------------------------------------+-
    //   | 4              + draft-ietf-hybi-thewebsocketprotocol-04 |
    //  -+----------------+-----------------------------------------+-
    //   | 5              + draft-ietf-hybi-thewebsocketprotocol-05 |
    //  -+----------------+-----------------------------------------+-
    //   | 6              + draft-ietf-hybi-thewebsocketprotocol-06 |
    //  -+----------------+-----------------------------------------+-
    //   | 7              + draft-ietf-hybi-thewebsocketprotocol-07 |
    //  -+----------------+-----------------------------------------+-
    //   | 8              + draft-ietf-hybi-thewebsocketprotocol-08 |
    //  -+----------------+-----------------------------------------+-
    //   | 9              + draft-ietf-hybi-thewebsocketprotocol-09 |
    //  -+----------------+-----------------------------------------+-
};

class WebSocketReceiver {
public:
    void receive(char* junk, std::size_t junkLen) {
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

    uint64_t readOpcode(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;
        if (junkLen > 0) {
            consumed++;

            uint8_t opCodeByte = *reinterpret_cast<uint8_t*>(junk);

            fin = opCodeByte & 0b10000000;
            opCode = (opCodeByte & 0b00001111) == 0 ? opCode : opCodeByte & 0b00001111;

            parserState = ParserState::LENGTH;
        }

        return consumed;
    }

    uint64_t readLength(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        if (junkLen > 0) {
            uint8_t lengthByte = *reinterpret_cast<uint8_t*>(junk);

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

    uint64_t readELength(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        elengthNumBytesLeft = (elengthNumBytesLeft == 0) ? elengthNumBytes : elengthNumBytesLeft;

        while (consumed < junkLen && elengthNumBytesLeft > 0) {
            length |= static_cast<uint64_t>(*reinterpret_cast<unsigned char*>(junk + consumed))
                      << (elengthNumBytes - elengthNumBytesLeft) * 8;

            consumed++;
            elengthNumBytesLeft--;
        }

        if (elengthNumBytesLeft == 0) {
            switch (elengthNumBytes) {
                case 2: {
                    length = be16toh(static_cast<uint16_t>(length));
                } break;
                case 8:
                    length = be64toh(length);
                    break;
            }
            parserState = ParserState::MASKINGKEY;
        }

        return consumed;
    }

    uint64_t readMaskingKey(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        if (masked) {
            maskingKeyNumBytesLeft = (maskingKeyNumBytesLeft == 0) ? maskingKeyNumBytes : maskingKeyNumBytesLeft;

            while (consumed < junkLen && maskingKeyNumBytesLeft > 0) {
                maskingKey |= static_cast<uint32_t>(*reinterpret_cast<unsigned char*>(junk + consumed))
                              << (maskingKeyNumBytes - maskingKeyNumBytesLeft) * 8;
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

    uint64_t readPayload(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        if (junkLen > 0 && length - payloadRead > 0) {
            uint64_t numBytesToRead = (junkLen <= length - payloadRead) ? junkLen : length - payloadRead;

            MaskingKeyAsArray maskingKeyAsArray = {.value = htobe32(maskingKey)};

            for (uint64_t i = 0; i < numBytesToRead; i++) {
                *(junk + i) = *(junk + i) ^ *(maskingKeyAsArray.array + (i + payloadRead) % 4);
            }

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

#define WSPAYLOADLENGTH 128

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

        MaskingKeyAsArray maskingKeyAsArray = {.value = htobe32(maskingKey)};

        for (uint64_t i = 0; i < payloadLength; i++) {
            *(frame + payloadOffset + i) = *(payload + i) ^ *(maskingKeyAsArray.array + i % 4);
        }

        for (std::size_t i = 0; i < frameLength; i++) {
            std::cout << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) (unsigned char) frame[i] << " ";

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

    [[maybe_unused]] const char* message = "Hallo Du, heute ist ein schöner Tag oder meinst du nicht?"
                                           "Hallo Du, heute ist ein schöner Tag oder meinst du nicht?"
                                           "Hallo Du, heute ist ein schöner Tag oder meinst du nicht?"
                                           "Hallo Du, heute ist ein schöner Tag oder meinst du nicht?";

    webSocketSender.send(1, message, std::string(message).length());

    std::string clientWebSocketKey = "dGhlIHNhbXBsZSBub25jZQ==";

    std::cout << std::dec << "Base64: " << serverWebSocketKey(clientWebSocketKey) << std::endl;
    std::cout << "Should: "
              << "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=" << std::endl;

    //    This section is non-normative.
    //    A single-frame unmasked text message
    //    0x81 0x05 0x48 0x65 0x6c 0x6c 0x6f (contains "Hello")
    //    A single-frame masked text message
    //    0x81 0x85 0x37 0xfa 0x21 0x3d 0x7f 0x9f 0x4d 0x51 0x58 (contains "Hello")
    //    A fragmented unmasked text message
    //    0x01 0x03 0x48 0x65 0x6c (contains "Hel")
    //    0x80 0x02 0x6c 0x6f (contains "lo")
    //    Ping request and response
    //    0x89 0x05 0x48 0x65 0x6c 0x6c 0x6f (contains a body of "Hello", but the contents of the body are arbitrary)
    //    0x8a 0x05 0x48 0x65 0x6c 0x6c 0x6f (contains a body of "Hello", matching the body of the ping)
    //    256 bytes binary message in a single unmasked frame
    //    0x82 0x7E 0x0100 [256 bytes of binary data]
    //    64KiB binary message in a single unmasked frame
    //    0x82 0x7F 0x0000000000010000 [65536 bytes of binary data]

    WebSocketReceiver webSocketReceiver;

    std::string s("\x81\x05\x48\x65\x6c\x6c\x6f");
    webSocketReceiver.receive(s.data(), s.length());

    s = "\x81\x85\x37\xfa\x21\x3d\x7f\x9f\x4d\x51\x58";
    webSocketReceiver.receive(s.data(), s.length());

    s = "\x01\x03\x48\x65\x6c";
    webSocketReceiver.receive(s.data(), s.length());
    s = "\x80\x02\x6c\x6f";
    webSocketReceiver.receive(s.data(), s.length());

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
