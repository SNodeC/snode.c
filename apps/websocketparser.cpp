#include "http/websocket/WSServerContext.h"
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

int main(int argc, char* argv[]) {
    SNodeC::init(argc, argv);

    http::websocket::WSServerContext wsTransCeiver;

    const char* message = "Hallo Du, heute ist ein schöner Tag oder meinst du nicht?"
                          "Hallo Du, heute ist ein schöner Tag oder meinst du nicht?"
                          "Hallo Du, heute ist ein schöner Tag oder meinst du nicht?"
                          "Hallo Du, heute ist ein schöner Tag oder meinst du nicht?";

    wsTransCeiver.message(1, message, std::string(message).length());

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

    std::string s("\x81\x05\x48\x65\x6c\x6c\x6f");
    wsTransCeiver.receive(s.data(), s.length());

    s = "\x81\x85\x37\xfa\x21\x3d\x7f\x9f\x4d\x51\x58";
    wsTransCeiver.receive(s.data(), s.length());

    s = "\x01\x03\x48\x65\x6c";
    wsTransCeiver.receive(s.data(), s.length());

    s = "\x80\x02\x6c\x6f";
    wsTransCeiver.receive(s.data(), s.length());

    wsTransCeiver.messageStart(1, message, std::string(message).length(), 0x12345678);
    wsTransCeiver.message(message, std::string(message).length(), 0x23456789);
    wsTransCeiver.message(message, std::string(message).length(), 0x12345678);
    wsTransCeiver.message(message, std::string(message).length(), 0x23456789);
    wsTransCeiver.messageEnd(message, std::string(message).length(), 0x34567890);

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
