#include "config.h"
#include "express/legacy/WebApp.h"
#include "express/tls/WebApp.h"
#include "log/Logger.h"
#include "net/SNodeC.h"
#include "net/timer/IntervalTimer.h"
#include "web/ws/WSProtocol.h"
#include "web/ws/server/WSContext.h"

#include <cstddef>
#include <endian.h>
#include <iomanip>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <vector>

using namespace express;
using namespace net;

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

char* serverWebSocketKey(const std::string& clientWebSocketKey) {
    std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    std::string serverWebSocketKey(clientWebSocketKey + GUID);
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(serverWebSocketKey.c_str()),
         serverWebSocketKey.length(),
         reinterpret_cast<unsigned char*>(&digest));

    return base64(digest, SHA_DIGEST_LENGTH);
}

void serverWebSocketKey(const std::string& clientWebSocketKey, const std::function<void(char*)>& returnKey) {
    std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    std::string serverWebSocketKey(clientWebSocketKey + GUID);
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(serverWebSocketKey.c_str()),
         serverWebSocketKey.length(),
         reinterpret_cast<unsigned char*>(&digest));

    char* key = base64(digest, SHA_DIGEST_LENGTH);
    returnKey(key);

    free(key);
}

int main(int argc, char* argv[]) {
    /*
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
    */

    SNodeC::init(argc, argv);

    legacy::WebApp legacyApp;

    legacyApp.get("/", [] MIDDLEWARE(req, res, next) {
        if (req.url == "/") {
            req.url = "/wstest.html";
        }

        if (req.url == "/ws") {
            next();
        } else {
            res.sendFile(CMAKE_SOURCE_DIR "apps/html" + req.url, [&req](int ret) -> void {
                if (ret != 0) {
                    PLOG(ERROR) << req.url;
                }
            });
        }
    });

    legacyApp.get("/ws", [](Request& req, Response& res) -> void {
        std::string uri = req.originalUrl;

        VLOG(1) << "OriginalUri: " << uri;
        VLOG(1) << "Uri: " << req.url;

        VLOG(1) << "Connection: " << req.header("connection");
        VLOG(1) << "Host: " << req.header("host");
        VLOG(1) << "Origin: " << req.header("origin");
        VLOG(1) << "Sec-WebSocket-Protocol: " << req.header("sec-websocket-protocol");
        VLOG(1) << "sec-web-socket-extensions: " << req.header("sec-websocket-extensions");
        VLOG(1) << "sec-websocket-key: " << req.header("sec-websocket-key");
        VLOG(1) << "sec-websocket-version: " << req.header("sec-websocket-version");
        VLOG(1) << "upgrade: " << req.header("upgrade");
        VLOG(1) << "user-agent: " << req.header("user-agent");

        res.set("Upgrade", "websocket");
        res.set("Connection", "Upgrade");
        res.set("Sec-WebSocket-Protocol", "echo");

        serverWebSocketKey(req.header("sec-websocket-key"), [&res](char* key) -> void {
            res.set("Sec-WebSocket-Accept", key);
        });

        res.status(101); // Switch Protocol

        res.upgrade("ws", "echo");
    });

    legacyApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            perror("Listen");
        } else {
            std::cout << "snode.c listening on port 8080" << std::endl;
        }
    });

    {
        tls::WebApp tlsApp({{"certChain", SERVERCERTF}, {"keyPEM", SERVERKEYF}, {"password", KEYFPASS}});

        tlsApp.get("/", [] MIDDLEWARE(req, res, next) {
            if (req.url == "/") {
                req.url = "/wstest.html";
            }

            if (req.url == "/ws") {
                next();
            } else {
                VLOG(0) << CMAKE_SOURCE_DIR "apps/html";
                res.sendFile(CMAKE_SOURCE_DIR "apps/html" + req.url, [&req](int ret) -> void {
                    if (ret != 0) {
                        PLOG(ERROR) << req.url;
                    }
                });
            }
        });

        tlsApp.get("/ws", [](Request& req, Response& res) -> void {
            std::string uri = req.originalUrl;

            VLOG(1) << "OriginalUri: " << uri;
            VLOG(1) << "Uri: " << req.url;

            VLOG(1) << "Connection: " << req.header("connection");
            VLOG(1) << "Host: " << req.header("host");
            VLOG(1) << "Origin: " << req.header("origin");
            VLOG(1) << "Sec-WebSocket-Protocol: " << req.header("sec-websocket-protocol");
            VLOG(1) << "sec-web-socket-extensions: " << req.header("sec-websocket-extensions");
            VLOG(1) << "sec-websocket-key: " << req.header("sec-websocket-key");
            VLOG(1) << "sec-websocket-version: " << req.header("sec-websocket-version");
            VLOG(1) << "upgrade: " << req.header("upgrade");
            VLOG(1) << "user-agent: " << req.header("user-agent");

            res.set("Upgrade", "websocket");
            res.set("Connection", "Upgrade");
            res.set("Sec-WebSocket-Protocol", "echo");

            serverWebSocketKey(req.header("sec-websocket-key"), [&res](char* key) -> void {
                res.set("Sec-WebSocket-Accept", key);
            });

            res.status(101); // Switch Protocol

            res.upgrade("ws", "echo");
        });

        tlsApp.listen(8088, [](int err) -> void {
            if (err != 0) {
                perror("Listen");
            } else {
                std::cout << "snode.c listening on port 8088" << std::endl;
            }
        });
    }

    return SNodeC::start();
}

/*
2021-05-14 10:21:39 0000000002:      accept-encoding: gzip, deflate, br
2021-05-14 10:21:39 0000000002:      accept-language: en-us,en;q=0.9,de-at;q=0.8,de-de;q=0.7,de;q=0.6
2021-05-14 10:21:39 0000000002:      cache-control: no-cache
2021-05-14 10:21:39 0000000002:      connection: upgrade
2021-05-14 10:21:39 0000000002:      host: localhost:8080
2021-05-14 10:21:39 0000000002:      origin: file://
2021-05-14 10:21:39 0000000002:      pragma: no-cache
2021-05-14 10:21:39 0000000002:      sec-websocket-extensions: permessage-deflate; client_max_window_bits
2021-05-14 10:21:39 0000000002:      sec-websocket-key: et6vtby1wwyooittpidflw==
2021-05-14 10:21:39 0000000002:      sec-websocket-version: 13
2021-05-14 10:21:39 0000000002:      upgrade: websocket
2021-05-14 10:21:39 0000000002:      user-agent: mozilla/5.0 (x11; linux x86_64) applewebkit/537.36 (khtml, like gecko)
chrome/90.0.4430.212 safari/537.36
*/
