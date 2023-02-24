# Simple NODE in C++ (SNode.C)

## What is SNode.C
[SNode.C](https://volkerchristian.github.io/snode.c-doc/html/index.html) is a very simple to use lightweight highly extendable event driven layer-based framework for network applications in the spirit of node.js written entirely in C\+\+.
The development of the  framework started during the summer semester 2020 in the context of the course **Network and Distributed Systems** of the masters program [**Interactive Media**](https://www.fh-ooe.at/en/hagenberg-campus/studiengaenge/master/interactive-media/) at the departement [**Informatics, Communications and Media**](https://www.fh-ooe.at/en/hagenberg-campus/) at the [**University of Applied Sciences Upper Austria, Campus Hagenberg**](https://www.fh-ooe.at/en/) to give students an insight into the fundamental techniques of network and web frameworks.
Main focus (but not only) of the framework is "Machine to Machine" (M2M) communication and here especially the field of "Internet of Things" (IoT).
Keep in mind, that the framework is still in heavy development, APIs can break from commit to commit. But the highest level API (express) is considered stable.

## Table of Contents

[TOC]

# License

SNode.C is released under the **GNU Lesser General Public License, Version 3** ([https://www.gnu.org/licenses/lgpl-3.0.de.html](https://www.gnu.org/licenses/lgpl-3.0.de.html))

# Copyright

(c) Volker Christian ([mailto://me@vchrist.at](mailto://me@vchrist.at))

Some components are also copyrighted by Students

- Json Middleware

  - Marlene Mayr

  - Anna Moser

  - Matteo Prock

  - Eric Thalhammer


- Regular-Expression Route-Mapping

  - Joelle Helgert

  - Julia Gruber

  - Patrick Brandstätter

  - Fabian Mohr

- MariaDB Database Support
  - Daniel Flockert

- OAuth2 Demo System
  - Daniel Flockert

# Quick Starting Guide

Basically the architecture of every server and client application is the same and consists of three components.

- Server and/or client instance
- SocketContextFactory
- SocketContext

Let's have a look at how these three components a related to each other by implementing a simple networking application.

## An "Echo" Application

Imagine we want to create a very basic TCP (**stream**)/IPv4 (**in**) server/client pair which sends some plain text data unencrypted (**legacy**) to each other in a ping-pong way.

The client shall start sending text data to the server and the server shall reflect that data back to the client. The client receives this reflected data and sends it back again to the server. This data ping-pong shall last infinitely long.

### SocketServer and SocketClient

For the server role we just need to create an object of type

```c++
net::in::stream::legacy::SocketServer<SocketContextFactory>
```

and for the client role an object of type

```c++
net::in::stream::legacy::SocketClient<SocketContextFactory>
```

is needed.

Both roles expect a class `SocketContextFactory`as template argument. Such a `SocketContextFactory` is used by the `SocketServer` and the `SocketClient` for creating a concrete `SocketContext` object for each established connection. This `SocketContext` represents the concrete application protocol.

Thus, for our echo application we need to implement the application logic (application protocol) for server and client in classes derived from `core::socket::stream::SocketContext`, which is the base class of all connection-oriented (stream) application protocols, and factories derived from `core::socket::stream::SocketContextFactory`.

### SocketContextFactories

Let's focus on the SocketContextFactories for our server and client first.

All what needs to be done is to implement a pure virtual method `create()`witch expects a pointer to a `core::socket::stream::SocketConnection` as argument and returns a concrete application SocketContext. 

The `core::socket::stream::SocketConnection` object involved is managed internally by the SNode.C and represents the physical connection between the server and a client. This `core::socket::stream::SocketConnection` is used by the `core::socket::stream::SocketContext` to handle the physical data transfer between server and client.

#### Echo-Server ContextFactory

The `create()` method of our `EchoServerContextFactory` returns the `EchoServerContext` whose implementation is presented in the SocketContext section below.

```c++
class EchoServerContextFactory : public core::socket::stream::SocketContextFactory {
private:
    core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
        return new EchoServerContext(socketConnection);
    }
};
```

#### Echo-Client ContextFactory

The `create()` method of our `EchoClientContextFactory` returns the `EchoClientContext` whose implementation is presented in the SocketContext section below.

```c++
class EchoClientContextFactory : public core::socket::stream::SocketContextFactory {
private:
    core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
        return new EchoClientContext(socketConnection);
    }  
};
```

That's easy, isn't it?

### SocketContexts

It is also not difficult to implement the SocketContext classes for the server and the client. 

- Remember, we need to derive from the base class `core::socket::stream::SocketContext`. 
- And also remember the required functionality: The server shall reflect the received data back to the client!
- And at last remember that `core::socket::stream::SocketContext` needs the `core::socket::stream::SocketConnection` to handle the physical data exchange. Thus, we have to pass the pointer to the SocketConnection to the constructor of the base `core::socket::stream::SocketContext` class.

The base class `core::socket::stream::SocketContext` provides some virtual methods which can be overridden in an concrete SocketContext class. These methods will be called by the framework automaticaly.

#### Echo-Server Context

For our echo server application it would be sufficient to override the `onReceivedFromPeer()` method only. This method is called by the framework in case some data have already been received from the client. Nevertheless, for more information of what is going on in behind the methods `onConnected` and `onDisconnected` are overridden also. 

```c++
class EchoServerContext : public core::socket::stream::SocketContext {
public:
    explicit EchoServerContext(core::socket::stream::SocketConnection* socketConnection) 
    	: core::socket::stream::SocketContext(socketConnection) {
    }

private: 
    void onConnected() override { // Called in case a connection has been established successfully.
    	std::cout << "Echo connected to " << socketConnection->getRemoteAddress().toString() << std::endl;
    }
    
    void onDisconnected() override { // Called in case the connection has been closed.
        std::cout << "Echo disconnected from " << socketConnection->getRemoteAddress().toString() << std::endl;
    }
    
    std::size_t onReceivedFromPeer() override { // Called in case data have already been received by the framework
                                                // and thus are ready for preccessing.
    	char junk[4096];

        std::size_t junkLen = readFromPeer(junk, 4096); // Fetch data.
                                                        // In case there are less than 4096 bytes available return at 
                                                        // least that amount of data.
                                                        // In case more than 4096 bytes are available 
                                                        // onReceivedFromPeer will be called again.
                                                        // No error can occure here.
        if (junkLen > 0) {
            std::cout << "Data to reflect: " << std::string(junk, junklen);
            sendToPeer(junk, junklen); // Reflect the received data back to the client.
                                       // Out of memory is the only error which can occure here.
        }

        return junkLen; // Return the amount of data processed to the framework.
    }
};
```

#### Echo-Client Context

The echo client SocketContext in contrast to the server SocketContext, *needs* an overridden `onConnected` method, to initiate the ping-pong data exchange.

```c++
class EchoClientContext : public core::socket::stream::SocketContext {
public:
    explicit EchoClientContext(core::socket::stream::SocketConnection* socketConnection) 
        : core::socket::stream::SocketContext(socketConnection) {
    }

private:
    void onConnected() override { // Called in case a connection has been established successfully.
        std::cout << "Echo connected to " << socketConnection->getRemoteAddress().toString() << std::endl;
        
        std::cout << "Initiating data exchange" << std::endl;
        sendToPeer("Hello peer! It's nice talking to you!!!"); // Initiate the ping-pong data exchange.
    }
    
    void onDisconnected() override { // Called in case the connection has been closed.
        std::cout << "Echo disconnected from " << socketConnection->getRemoteAddress().toString() << std::endl;
    }
    
    std::size_t onReceivedFromPeer() override { // Called in case data have already been received by the framework
                                                // and thus are ready for preccessing.
        char junk[4096];

        std::size_t junkLen = readFromPeer(junk, 4096); // Fetch data.
                                                        // In case there are less than 4096 bytes available return at 
                                                        // least that amount of data.
                                                        // In case more than 4096 bytes are available 
                                                        // onReceivedFromPeer will be called again.
                                                        // No error can occure here.
        if (junkLen > 0) {
            std::cout << "Data to reflect: " << std::string(junk, junklen);
            sendToPeer(junk, junklen); // Reflect the received data back to the server.
                                       // Out of memory is the only error which can occure here.
        }

        return junkLen; // Return the amount of data processed to the framework.
    }
};
```

### Main Applications for server and client

Now we can put all together and implement the server and client main applications.

#### Echo-Server Main Application

```c++
int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv); // Initialize the framework.
                                    // Configure logging, create command line arguments, daemonize if requested.
    
    using EchoServer = net::in::stream::legacy::SocketServer<EchoServerContextFactory>; // Simplify data type
                                                                                                // Note the use of our implemented
                                                                                                // EchoServerContextFactory as
                                                                                                // template argument
    using SocketAddress = EchoServer::SocketAddress; // Simplify data type
    
    EchoServer echoServer; // Create server instance
    
    echoServer.listen(8001, [](const SocketAddress& socketAddress, int err) -> void { // Listen on port 8001 on all interfaces
        if (err == 0){
            std::cout << "Success: Echo server listening on " << socketAddress.toString() << std::endl;
        } else {
            std::cout << "Error: Echo server listening on " << socketAddress.toString() << ": " << perror("") << std::endl;
        }
    });
    
    return core::SNodeC::start(); // Start the event loop.
}
```

#### Echo-Client Main Application

```c++
int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv); // Initialize the framework.
                                    // Configure logging, create command line arguments, daemonize if requested.
    
    using EchogClient = net::in::stream::legacy::SocketClient<EchoClientContextFactory>; // Simplify data type
                                                                                                // Note the use of our implemented
                                                                                                // EchoClientContextFactory as
                                                                                                // template argument
    using SocketAddress = EchogClient::SocketAddress; // Simplify data type
    
    EchogClient echoClient; // Create client instance
    
    echoClient.connect("localhost", 8001, [](const SocketAddress& socketAddress, int err) -> void { // Connect to server
        if (err == 0){
            std::cout << "Success: Echo connecting to " << socketAddress.toString() << std::endl;
        } else {
            std::cout << "Error: Echo client connecting to " << socketAddress.toString() << ": " << perror("");
        }
    });
    
    return core::SNodeC::start(); // Start the event loop.
}
```

## Summary

The echo application shows the typical architecture of every server and client applications implemented using SNode.C.

The user needs to provide the application protocol layer by implementing the classes

- SocketContextFactory
- SocketContext

This classes need to be derived from the base classes

- `core::socket::stream::SocketContextFactory`
- `core::socket::stream::SocketContext`

The framework provides

- ready to use server and client template classes for each network layer/transport layer combination.

# Installation

SNode.C depends on some external libraries. Some of these libraries are directly included in the framework.

## Supported Systems and Hardware

The main development of SNode.C takes place on an debian style linux system. Though, it should compile cleanly on every linux system provided that all required tools and libraries are installed.

SNode.C is known to compile run cleanly on

- x86 architectures (32 and 64 bit)
  - Tested on HP ZBook 15 G8
- Arm architectures (32 and 64 bit)
  - Tested on Raspberry Pi

## Tools

### Required

- git ([https://git-scm.com/](https://git-scm.com/))
- cmake ([https://cmake.org/](https://cmake.org/))
- make ([https://www.gnu.org/software/make/#:~:text=GNU%20Make%20is%20a%20tool,compute%20it%20from%20other%20files.](https://www.gnu.org/software/make/#:~:text=GNU%20Make%20is%20a%20tool,compute%20it%20from%20other%20files.)) or 
- ninja ([https://ninja-build.org/](https://ninja-build.org/))
- g++ ([https://gcc.gnu.org/](https://gcc.gnu.org/)) or 
- clang ([https://clang.llvm.org/](https://clang.llvm.org/))
- pkg-config ([https://www.freedesktop.org/wiki/Software/pkg-config/](https://www.freedesktop.org/wiki/Software/pkg-config/))

### Optional

- iwyu ([https://include-what-you-use.org/](https://include-what-you-use.org/))
- clang-format ([https://clang.llvm.org/docs/ClangFormat.html](https://clang.llvm.org/docs/ClangFormat.html))
- cmake-format ([https://cmake-format.readthedocs.io/en/latest/cmake-format.html](https://cmake-format.readthedocs.io/en/latest/cmake-format.html))
- doxygen ([https://www.doxygen.nl/](https://www.doxygen.nl/))

## Libraries

### Required

- Easylogging ([https://github.com/amrayn/easyloggingpp](https://github.com/amrayn/easyloggingpp))
- OpenSSL ([https://www.openssl.org/](https://www.openssl.org/))
- Nlohmann-JSON ([https://json.nlohmann.me/](https://json.nlohmann.me/))

### Optional

- Bluez development files ([http://www.bluez.org/](http://www.bluez.org/))
- LibMagic development files ([https://www.darwinsys.com/file/](https://www.darwinsys.com/file/))
- MariaDB client development files ([https://mariadb.org/](https://mariadb.org/))

### In Framework

This components are already integrated directly in SNode.C. Thus they need not be installed by hand

- CLI11 ([https://github.com/CLIUtils/CLI11](https://github.com/CLIUtils/CLI11))

## Installation on debian style systems (x86, Arm)

### Dependencies

To install all dependencies on debian style systems just run

```sh
sudo apt update
sudo apt install git cmake make ninja-build g++ clang pkg-config
sudo apt install iwyu clang-format cmake-format doxygen
sudo apt install libeasyloggingpp-dev libssl-dev nlohmann-json3-dev
sudo apt install libbluetooth-dev libmagic-dev libmariadb-dev
```

### SNode.C

After installing alle dependencies SNode.C can be cloned from github and compiled

```sh
mkdir snode.c
cd snode.c
git clone https://github.com/VolkerChristian/snode.c.git
mkdir build
cd build
cmake ../snode.c
make
sudo make install
```

# Design Decisions and Features

- Object orientated
- Single-threaded, single-tasking
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

Currently only connection-oriented protocols (SOCK_STREAM) for all network layer protocols are supported. For IPv4 and IPv6 this means TCP.

- Every transport layer protocol provides a common base API which makes it very easy to create servers and clients for the different network layers.
- New application protocols can be connected to the transport layer very easily.
- Transparently offers SSL/TLS encryption provided by openssl for each supportet network protocol and thus, also for all application level protocols.
  - Support of X.509 certificates. TLS/SSL connections can be protected and authenticated using certificates.
  - Server Name Indication is supported.


## Application Layer

In-framework support currently exist for the application level protocols

- HTTP/1.1
- WebSocket version 13
- MQTT version 3.1.1 (version 5.0 is in preparation)
- MQTT via WebSockets
- High-Level Web API layer with JSON support very similar to the API of node.js/express.

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