# Simple NODE in C++ (SNode.C)

## What is SNode.C
[SNode.C](https://volkerchristian.github.io/snode.c-doc/html/index.html) is a very simple to use lightweight highly extendable event driven layer-based framework for network applications in the spirit of node.js written entirely in C\+\+.
The development of the  framework started during the summer semester 2020 in the context of the course **Network and Distributed Systems** of the masters program [**Interactive Media**](https://www.fh-ooe.at/en/hagenberg-campus/studiengaenge/master/interactive-media/) at the departement [**Informatics, Communications and Media**](https://www.fh-ooe.at/en/hagenberg-campus/) at the [**University of Applied Sciences Upper Austria, Campus Hagenberg**](https://www.fh-ooe.at/en/) to give students an insight into the fundamental techniques of network and web frameworks.
Main focus (but not only) of the framework is "Machine to Machine" (M2M) communication and here especially the field of "Internet of Things" (IoT).
Keep in mind, that the framework is still in heavy development, APIs can break from commit to commit. But the highest level API (express) is considered stable.

## Quick Starting Guide

Imagine you want to create a very basic TCP/IPv4 server/client pair which sends some plain text data unencrypted to each other in a ping-pong way. The client shall start sending text data to the server and the server shall reflect that data back to the client and the client should also reflect the received data back to the server. This data ping-pong shall be done infinitely long.

For the server role you just need to create an object of type

```c++
net::ipv4::stream::legacy::SocketServer<SocketContextFactory>
```

and for the client role an object of type

```c++
net::ipv4::stream::legacy::SocketClient<SocketContextFactory>
```

is needed.

Both roles expect a class `SocketContextFactory`as template argument. A `SocketContextFactory` is used by the `SocketServer` and the `SocketClient` to creating a concrete `SocketContext` object for each established connection. A `SocketContext` represents a concrete application protocol.

Thus, for our Ping-Pong application we need to implement the application logic for server and client in classes derived from `core::socket::stream::SocketContext`what is the base class of all connection oriented (stream) application protocols, and factories derived from`core::socket::stream::SocketContextFactory`.

### SocketContextFactories

Let's focus on the `SocketContextFactories`for server and client first.

All what needs to be done is to implement a pure virtual method `create(core::socket::stream::SocketConnection* socketConnection)`witch expects a pointer to a `core::socket::stream::SocketConnection` as argument. Such a `core::socket::stream::SocketConnection` object represents the physical connection between the server and a client. This `core::socket::stream::SocketConnection` is needed by an `core::socket::stream::SocketContext` to handle the physical data transfer.

#### PingPong-Server ContextFactory

```c++
class PingPongServerContextFactory : public core::socket::stream::SocketContextFactory {
private:
    core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
        return new PingPongServerContext(socketConnection);
    }
};
```

#### PingPong-Client ContextFactory

```c++
class PingPongClientContextFactory : public core::socket::stream::SocketContextFactory {
private:
    core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
        return new PingPongClientContext(socketConnection);
    }  
};
```

That's easy, isn't it?

### SocketContexts

It is also not difficult to implement the SocketContext classes for the server and the client. Remember, we need to derive from the base `core::socket::stream::SocketContext`.

#### PingPong-Server Context

```c++
class PingPongServerContext : public core::socket::stream::SocketContext {
public:
    explicit PingPongServerContext(core::socket::stream::SocketConnection* socketConnection) 
    	: core::socket::stream::SocketContext(socketConnection) {
    }
private: 
    void onConnected() override { // called in case a connection has been established successfully
    	std::cout << "PingPong server connected to " << socketConnection->getRemoteAddress().toString() << std::endl;
    }
    
    void onDisconnected() override { // called in case the connection has been closed
        std::cout << "PingPong server disconnected from " << socketConnection->getRemoteAddress().toString() << std::endl;
    }
    
    std::size_t onReceiveFromPeer() override { // called in case data have already been received by the framework
                                               // and thus are ready for preccessing
    	char junk[4096];

        std::size_t junkLen = readFromPeer(junk, 4096);

        if (junkLen > 0) {
            std::cout << "Data to reflect: " << std::string(junk, junklen);
            sendToPeer(junk, junklen);
        }

        return junkLen;
    }
};
```

#### PingPong-Client Context

```c++
class PingPongClientContext : public core::socket::stream::SocketContext {
public:
    explicit PingPongClientContext(core::socket::stream::SocketConnection* socketConnection) 
        : core::socket::stream::SocketContext(socketConnection)
    
    void onConnected() override { // called in case a connection has been established successfully
        std::cout << "PingPong client connected to " << socketConnection->getRemoteAddress().toString() << std::endl;
        std::cout << "Initiating data exchange" << std::endl;
        
        sendToPeer("Hello peer! Nice to talk to you!!!");
    }
    
    void onDisconnected() override { // called in case the connection has been closed
        std::cout << "PingPong client disconnected from " << socketConnection->getRemoteAddress().toString() << std::endl;
    }
    
    std::size_t onReceiveFromPeer() override { // called in case data have already been received by the framework
                                               // and thus are ready for preccessing
        char junk[4096];

        std::size_t junkLen = readFromPeer(junk, 4096);

        if (junkLen > 0) {
            std::cout << "Data to reflect: " << std::string(junk, junklen);
            sendToPeer(junk, junklen);
        }

        return junkLen;
    }
};
```

### Main Applications for server and client

Now we can put all together and implement the server and client main applications.

#### PingPong-Server Main Application

```c++
int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    
    using PingPongServer = net::ipv4::stream::legacy::SocketServer<PingPongServerContextFactory>;
    using SocketAddress = PingPongServer::SocketAddress;
    
    PingPongServer pingPongServer;
    
    pingPongServer.listen(8001, [](const SocketAddress& socketAddress, int err) -> void {
        if (err == 0){
            std::cout << "Success: PingPong server listening on " << socketAddress.toString() << std::endl;
        } else {
            std::cout << "Error: PingPong server listening on " << socketAddress.toString() << ": " << perror("") << std::endl;
        }
    });
    
    return core::SNodeC::start();
}
```

#### PingPong-Client Main Application

```c++
int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    
    using PingPongClient = net::ipv4::stream::legacy::SocketClient<PingPongClientContextFactory>;
    using SocketAddress = PingPongServer::SocketAddress;
    
    PingPongClient pingPongClient;
    
    pingPongClient.connect("localhost", 8001, [](const SocketAddress& socketAddress, int err) -> void {
        if (err == 0){
            std::cout << "Success: PingPong connecting to " << socketAddress.toString() << std::endl;
        } else {
            std::cout << "Error: PingPong client connecting to " << socketAddress.toString() << ": " << perror("");
        }
    });
    
    return core::SNodeC::start();
}
```

## Initial Design Decisions

- Single-threaded, single-tasking,
- Event driven (asynchronous)
- Layer based
- Support for single shot and interval timer 
- Modular

## Network Layer

- Internet Protocol version 4 (IPv4)
- Internet Protocol version 6 (IPv6)
- Unix Domain Sockets
- Bluetooth Radio Frequency Communication (RFCOMM)
- Bluetooth Logical Link Control and Adaptation Protocol (L2CAP)

## Transport Layer

Currently only connection oriented protocols (SOCK_STREAM) for all network layer protocols are supported. For IPv4 and IPv6 this means TCP.

Every transport layer provide a common API which makes it very easy to create servers and clients for different network and supports unencrypted and SSL/TLS encrypted communication.

The transport layer is totally application protocol agnostic.

Application protocols like e.g. HTTP(S) can be connected very easily

But a rudimentary datagram 

- Application protocol independent TCP server and TCP client functionality
  Application protocols can be connected easily and modularly.
  Currently in-framework support exists for
  - HTTP and WebSocket
  - MQTT and MQTT via WebSockets

- Application protocol independent bluetooth RFCOMM and L2CAP server and client functionality. Application protocols can be easily and modularly connected.
- TLS/SSL encryption for server and client (TCP, RFCOMM).
- Support of X.509 certificates. TLS/SSL connections can be protected and authenticated using certificates (TCP, RFCOMM).
- Fully implemented HTTP(S) application protocol layer (server and client) to which the application logic can be easily connected.
- High-Level Web API layer with JSON support similar to the API of node.js/express. (Speedup compared to node.js/express approx. 40)

## Copyright
GNU Lesser General Public License

### Initiator and Main developer
- Volker Christian (me_AT_vchrist.at, volker.christian_AT_fh-hagenberg.at)

### Contributors (Students)
#### Json Middleware
- Marlene Mayr
- Anna Moser
- Matteo Prock
- Eric Thalhammer

#### Regular-Expression Route-Mapping
- Joelle Helgert
- Julia Gruber
- Patrick Brandstätter
- Fabian Mohr

#### Runtime Module Loading

## Requirements
- On Debian-Style-Systems run  
    **sudo apt install doxygen iwyu clang-format cmake-format libmagic-dev libbackward-cpp-dev
    libdw-dev libdwarf-dev binutils-dev librange-v3-dev libssl-dev libeasyloggingpp-dev libbluetooth-dev nlohmann-json3-dev**  
  to install all neccesarry dependencies.
- cmake-format
- doxygen
- libeasyloggingpp-dev
- clang-format
- libbackward-cpp-dev
- binutils-dev libdw-dev libunwind-dev
- libbluetooth-dev
- libssl-dev
- nlohmann-json3-dev
- libmagic-dev
- iwyu
- cmake

## Components
- libnet (directory net) low level multiplexed network-layer (server/client) with event-loop supporting legacy- and tls-connections
- libhttp (directory http) low level http server and client
- libexpress (directory express) high level web-api similar to node.js's express module
- example applications (directory apps)

## TODOs
- Add better error processing in HTTPResponseParser
- Extend regex-match in Router (path-to-regex in JS)
- Add some more complex example apps (Game, Skill for Alexa, ...)
- Add WebSocket support for server and client, legacy and tls
- Add support for Template-Engines (similar to express)
- Add additional transport protocols (udp, bluetooth, local, ipv6, ...)
- Implement other protocols (contexts) than http (e.g. mqtt, ftp would be interresting, telnet, smtp)
- Finalize cmake-rules to support install
- Add cmake description files
- Add "logical" events
- Support "promisses"
- Add OAUTH2 server/client authentification
- Write unit tests
- Porting snode.c to MS-Windows
- Porting snode.c to macOS
- Add onSSLConnect callback

## Example Applications
### Very Simple HTTP/S Web-Server for static HTML-Pages
```cpp
#include <easylogging++.h>

#include "legacy/WebApp.h"
#include "tls/WebApp.h"
#include "middleware/StaticMiddleware.h"

#define CERTF <PATH_TO_SERVER_CERTIFICATE_CHAIN_FILE>
#define KEYF <PATH_TO_SERVER_CERTIFICATE_KEY_FILE>
#define KEYFPASS <PASSWORD_OF_THE_SERVER_CERTIFICATE_KEY_FILE>

#define SERVERROOT <PATH_TO_ROOT_OF_WEBSITE>

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);
    
    express::legacy::WebApp legacyApp;
    legacyApp.use(express::middleware::StaticMiddleware(SERVERROOT));
    
    express::tls::WebApp tlsApp({{"certChain", CERTF}, 
                                 {"keyPEM", KEYF}, 
                                 {"password", KEYFPASS}});
    tlsApp.use(express::middleware::StaticMiddleware(SERVERROOT));
    
    legacyApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8080 for legacy connections";
        }
    });
    
    tlsApp.listen(8088, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8088";
        } else {
            VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
        }
    });
    
    return express::WebApp::start();
}
```

### Receive Data via HTTP-Post Request
```cpp
#include <easylogging++.h>

#include "legacy/WebApp.h"
#include "tls/WebApp.h"

#define CERTF <PATH_TO_SERVER_CERTIFICATE_CHAIN_FILE>
#define KEYF <PATH_TO_SERVER_CERTIFICATE_KEY_FILE>
#define KEYFPASS <PASSWORD_OF_THE_SERVER_CERTIFICATE_KEY_FILE>

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);

    express::legacy::WebApp legacyApp;
    legacyApp.get("/", [] APPLICATION(req, res) {
        res.send("<html>"
                 "    <head>"
                 "        <style>"
                 "            main {"
                 "                min-height: 30em;"
                 "                padding: 3em;"
                 "                background-image: repeating-radial-gradient( circle at 0 0, #fff, #ddd 50px);"
                 "            }"
                 "            input[type=\"file\"] {"
                 "                display: block;"
                 "                margin: 2em;"
                 "                padding: 2em;"
                 "                border: 1px dotted;"
                 "            }"
                 "        </style>"
                 "    </head>"
                 "    <body>"
                 "        <h1>Datei-Upload mit input type=\"file\"</h1>"
                 "        <main>"
                 "            <h2>Schicken Sie uns was Schickes!</h2>"
                 "            <form method=\"post\" enctype=\"multipart/form-data\">"
                 "                <label> Wählen Sie eine Textdatei (*.txt, *.html usw.) von Ihrem Rechner aus."
                 "                    <input name=\"datei\" type=\"file\" size=\"50\" accept=\"text/*\">"
                 "                </label>"
                 "                <button>… und ab geht die Post!</button>"
                 "            </form>"
                 "        </main>"
                 "    </body>"
                 "</html>");
    });

    legacyApp.post("/", [] APPLICATION(req, res) {
        VLOG(0) << "Content-Type: " << req.header("Content-Type");
        VLOG(0) << "Content-Length: " << req.header("Content-Length");
        char* body = new char[std::stoul(req.header("Content-Length")) + 1];
        memcpy(body, req.body, std::stoul(req.header("Content-Length")));
        body[std::stoi(req.header("Content-Length"))] = 0;

        VLOG(0) << "Body: ";
        VLOG(0) << body;
        
        delete[] body;
        
        res.send("<html>"
                 "    <body>"
                 "        <h1>Thank you</h1>"
                 "    </body>"
                 "</html>");
    });

    express::tls::WebApp tlsWebApp({{"certChain", CERTF},
                                    {"keyPEM", KEYF},
                                    {"password", KEYFPASS}});
    tlsWebApp.use(legacyApp);

    legacyApp.listen(8080, [](int err) {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8080 for legacy connections";
        }
    });

    tlsWebApp.listen(8088, [](int err) {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8088";
        } else {
            VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
        }
    });

    return express::WebApp::start();
}
```

### Extract Server and Client Information (host name, IP, port, SSL/TLS information)
```cpp
#include <easylogging++.h>
#include <openssl/x509v3.h>

#include "legacy/WebApp.h"
#include "tls/WebApp.h"
#include "middleware/StaticMiddleware.h"

#define CERTF <PATH_TO_SERVER_CERTIFICATE_CHAIN_FILE>
#define KEYF <PATH_TO_SERVER_CERTIFICATE_KEY_FILE>
#define KEYFPASS <PASSWORD_OF_THE_SERVER_CERTIFICATE_KEY_FILE>

#define SERVERROOT <PATH_TO_ROOT_OF_WEBSITE>

using namespace express;

Router getRouter() {
    Router router;

    router.use(middleware::StaticMiddleware(SERVERROOT));

    return router;
}

int main(int argc, char** argv) {
    WebApp::init(argc, argv);

    legacy::WebApp legacyApp(getRouter());

    legacyApp.listen(8080, [](int err) {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8080 for legacy connections";
        }
    });

    legacyApp.onConnect([](core::socket::legacy::SocketConnection* socketConnection) {
        VLOG(0) << "OnConnect:";
        VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().host()
                          + "(" + socketConnection->getRemoteAddress().ip() + "):"
                 + std::to_string(socketConnection->getRemoteAddress().port());
        VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().host()
                          + "(" + socketConnection->getLocalAddress().ip() + "):"
                 + std::to_string(socketConnection->getLocalAddress().port());
    });

    legacyApp.onDisconnect([](core::socket::legacy::SocketConnection* socketConnection) {
        VLOG(0) << "OnDisconnect:";
        VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().host()
                          + "(" + socketConnection->getRemoteAddress().ip() + "):"
                 + std::to_string(socketConnection->getRemoteAddress().port());
        VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().host() 
                          + "(" + socketConnection->getLocalAddress().ip() + "):"
                 + std::to_string(socketConnection->getLocalAddress().port());
    });

    tls::WebApp tlsApp(getRouter(), {{"certChain", CERTF}, 
                                     {"keyPEM", KEYF}, 
                                     {"password", KEYFPASS}});

    tlsApp.listen(8088, [](int err) {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8088";
        } else {
            VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
        }
    });

    tlsApp.onConnect([](core::socket::tls::SocketConnection* socketConnection) {
        VLOG(0) << "OnConnect:";
        VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().host()
                          + "(" + socketConnection->getLocalAddress().ip() + "):"
                 + std::to_string(socketConnection->getLocalAddress().port());
        VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().host()
                          + "(" + socketConnection->getRemoteAddress().ip() + "):"
                 + std::to_string(socketConnection->getRemoteAddress().port());

        X509* client_cert = SSL_get_peer_certificate(socketConnection->getSSL());

        if (client_cert != NULL) {
            int verifyErr = SSL_get_verify_result(socketConnection->getSSL());

            VLOG(0) << "\tClient certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

            char* str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
            VLOG(0) << "\t   Subject: " + std::string(str);
            OPENSSL_free(str);

            str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
            VLOG(0) << "\t   Issuer: " + std::string(str);
            OPENSSL_free(str);

            // We could do all sorts of certificate verification stuff here before deallocating the certificate.

            GENERAL_NAMES* subjectAltNames = static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(client_cert, NID_subject_alt_name, NULL, NULL));

            int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
            VLOG(0) << "\t   Subject alternative name count: " << altNameCount;
            for (int32_t i = 0; i < altNameCount; ++i) {
                GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
                if (generalName->type == GEN_URI) {
                    std::string subjectAltName = std::string(reinterpret_cast<const char*> (ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)), ASN1_STRING_length(generalName->d.uniformResourceIdentifier));
                    VLOG(0) << "\t      SAN (URI): '" + subjectAltName;
                } else if (generalName->type == GEN_DNS) {
                    std::string subjectAltName = std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)), ASN1_STRING_length(generalName->d.dNSName));
                    VLOG(0) << "\t      SAN (DNS): '" + subjectAltName;
                } else {
                    VLOG(0) << "\t      SAN (Type): '" + std::to_string(generalName->type);
                }
            }
            sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);

            X509_free(client_cert);
        } else {
            VLOG(0) << "\tClient certificate: no certificate";
        }
    });

    tlsApp.onDisconnect([](core::socket::tls::SocketConnection* socketConnection) {
        VLOG(0) << "OnDisconnect:";
        VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().host()
                          + "(" + socketConnection->getLocalAddress().ip() + "):"
                 + std::to_string(socketConnection->getLocalAddress().port());
        VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().host()
                          + "(" + socketConnection->getRemoteAddress().ip() + "):"
                 + std::to_string(socketConnection->getRemoteAddress().port());
    });

    return WebApp::start();
}
```

### Using Regular Expressions in Routes
```cpp
#include "legacy/WebApp.h"
#include "tls/WebApp.h"

#include <easylogging++.h>

using namespace express;

Router router() {
    Router router;

    // http://localhost:8080/account/123/username
    router.get("/account/:userId(\\d*)/:username", [] APPLICATION(req, res) {
        VLOG(0) << "Show account of";
        VLOG(0) << "UserId: " << req.params["userId"];
        VLOG(0) << "UserName: " << req.params["userName"];

        std::string response = "<html>"
                               "  <head>"
                               "    <title>Response from snode.c</title>"
                               "  </head>"
                               "  <body>"
                               "    <h1>Regex return</h1>"
                               "    <ul>"
                               "      <li>UserId: " + req.params["userId"] + </li>"
                               "      <li>UserName: " + req.params["userName"] + </li>"
                               "    </ul>"
                               "  </body>"
                               "</html>";
        res.send(response);
    });

    // http://localhost:8080/asdf/d123e/jklm/hallo
    router.get("/asdf/:regex1(d\\d{3}e)/jklm/:regex2", [] APPLICATION(req, res) {
        VLOG(0) << "Testing Regex";
        VLOG(0) << "Regex1: " << req.params["regex1"];
        VLOG(0) << "Regex2: " << req.params["regex2"];

        std::string response = "<html>"
                               "  <head>"
                               "    <title>Response from snode.c</title>"
                               "  </head>"
                               "  <body>"
                               "    <h1>Regex return</h1>"
                               "    <ul>"
                               "      <li>Regex 1: " + req.params["regex1"] + </li>"
                               "      <li>Regex 2: " + req.params["regex2"] + </li>"
                               "    </ul>"
                               "  </body>"
                               "</html>";

        res.send(response);
    });

    // http://localhost:8080/search/QueryString
    router.get("/search/:search", [] APPLICATION(req, res) {
        VLOG(0) << "Show Search of";
        VLOG(0) << "Search: " + req.params["search"];

        res.send(req.params["search"]);
    });

    router.use([] APPLICATION(req, res) {
        res.status(404).send("Not found: " + req.url);
    });

    return router;
}
```