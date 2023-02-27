# Simple NODE in C++ (SNode.C)

[SNode.C](https://volkerchristian.github.io/snode.c-doc/html/index.html) is a very simple to use lightweight highly extendable event driven layer-based framework for network applications in the spirit of node.js written entirely in C\+\+.

The development of the  framework started during the summer semester 2020 in the context of the course **Network and Distributed Systems** of the masters program [**Interactive Media**](https://www.fh-ooe.at/en/hagenberg-campus/studiengaenge/master/interactive-media/) at the departement [**Informatics, Communications and Media**](https://www.fh-ooe.at/en/hagenberg-campus/) at the [**University of Applied Sciences Upper Austria, Campus Hagenberg**](https://www.fh-ooe.at/en/) to give students an insight into the fundamental techniques of network and web frameworks.

Main focus (but not only) of the framework is "Machine to Machine" (M2M) communication and here especially the field of "Internet of Things" (IoT).

# Table of Content
<!--ts-->
* [Simple NODE in C++ (SNode.C)](#simple-node-in-c-snodec)
* [Table of Content](#table-of-content)
* [License](#license)
* [Copyright](#copyright)
* [Quick Starting Guide](#quick-starting-guide)
   * [An "Echo" Application](#an-echo-application)
      * [SocketServer and SocketClient](#socketserver-and-socketclient)
      * [SocketContextFactories](#socketcontextfactories)
         * [Echo-Server ContextFactory](#echo-server-contextfactory)
         * [Echo-Client ContextFactory](#echo-client-contextfactory)
      * [SocketContexts](#socketcontexts)
         * [Echo-Server Context](#echo-server-context)
         * [Echo-Client Context](#echo-client-context)
      * [Main Applications for server and client](#main-applications-for-server-and-client)
         * [Echo-Server Main Application](#echo-server-main-application)
         * [Echo-Client Main Application](#echo-client-main-application)
   * [Summary](#summary)
* [Installation](#installation)
   * [Minimum required Compiler Versions](#minimum-required-compiler-versions)
   * [Supported Systems and Hardware](#supported-systems-and-hardware)
   * [Tools](#tools)
      * [Required](#required)
      * [Optional](#optional)
   * [Libraries](#libraries)
      * [Required](#required-1)
      * [Optional](#optional-1)
      * [In-Framework](#in-framework)
   * [Installation on Debian Style Systems (x86, Arm)](#installation-on-debian-style-systems-x86-arm)
      * [Dependencies](#dependencies)
      * [SNode.C](#snodec)
* [Design Decisions and Features](#design-decisions-and-features)
   * [Network Layer](#network-layer)
   * [Transport Layer](#transport-layer)
   * [Application Layer](#application-layer)
* [Example Applications](#example-applications)
   * [HTTP/S Web-Server for Static HTML-Pages](#https-web-server-for-static-html-pages)
   * [Receive Data via HTTP-Post Request](#receive-data-via-http-post-request)
   * [Extract Server and Client Information (host name, IP, port, SSL/TLS information)](#extract-server-and-client-information-host-name-ip-port-ssltls-information)
   * [Using Regular Expressions in Routes](#using-regular-expressions-in-routes)

<!-- Added by: runner, at: Mon Feb 27 00:44:09 UTC 2023 -->

<!--te-->

# License

SNode.C is released under the **GNU Lesser General Public License, Version 3** ([<https://www.gnu.org/licenses/lgpl-3.0.de.html>](https://www.gnu.org/licenses/lgpl-3.0.de.html))

# Copyright

Volker Christian ([me@vchrist.at](mailto:me@vchrist.at) or [Volker.Christian@fh-hagenberg.at](mailto:volker.christian@fh-hagenberg.at))

Some components are also copyrighted by Students

-   Json Middleware

    -   Marlene Mayr

    -   Anna Moser

    -   Matteo Prock

    -   Eric Thalhammer

-   Regular-Expression Route-Mapping

    -   Joelle Helgert

    -   Julia Gruber

    -   Patrick Brandstätter

    -   Fabian Mohr

-   MariaDB Database Support

    -   Daniel Flockert

-   OAuth2 Demo System

    -   Daniel Flockert

# Quick Starting Guide

Basically the architecture of every server and client application is the same and consists of three components.

-   Server respective client instance
-   SocketContextFactory
-   SocketContext

Let\'s have a look at how these three components a related to each other by implementing a simple networking application.

## An \"Echo\" Application

Imagine we want to create a very basic TCP (**stream**)/IPv4 (**in**) server/client pair which sends some plain text data unencrypted (**legacy**) to each other in a ping-pong way.

The client shall start sending text data to the server and the server shall reflect that data back to the client. The client receives this reflected data and sends it back again to the server. This data ping-pong shall last infinitely long.

### SocketServer and SocketClient

For the server role we just need to create an object of type

``` c++
net::in::stream::legacy::SocketServer<SocketContextFactory>
```

called a *server instance* and for the client role an object of type

``` c++
net::in::stream::legacy::SocketClient<SocketContextFactory>
```

called *client instance* is needed.

Both role classes have a default constructor and a constructor expecting an instance name as argument. 

- When the default constructor is used to create the instance object this instance is called an *unnamed instance*, in contrast to a *named instance* if the constructors expecting a `std::string` is used for object creation. 
- For named instances command line arguments and configuration file entries are created automatically to configure the instance.

A class `SocketContextFactory` is used for both roles as template argument. Such a `SocketContextFactory` is used internally by the `SocketServer` and the `SocketClient` for creating a concrete `SocketContext` object for each established connection. This `SocketContext` represents a concrete application protocol.

Thus, for our echo application we need to implement the application logic (application protocol) for server and client in classes derived from `core::socket::stream::SocketContext`, which is the base class of all connection-oriented (stream) application protocols, and factories derived from `core::socket::stream::SocketContextFactory`.

### SocketContextFactories

Let\'s focus on the SocketContextFactories for our server and client first.

All what needs to be done is to implement a pure virtual method `create()`witch expects a pointer to a `core::socket::stream::SocketConnection` as argument and returns a concrete application SocketContext.

The `core::socket::stream::SocketConnection` object involved is managed internally by SNode.C and represents the physical connection between the server and a client. This `core::socket::stream::SocketConnection` is used by the `core::socket::stream::SocketContext` to handle the physical data transfer between server and client.

#### Echo-Server ContextFactory

The `create()` method of our `EchoServerContextFactory` returns the `EchoServerContext` whose implementation is presented in the [SocketContexts](#SocketContexts) section below.

``` c++
class EchoServerContextFactory : public core::socket::stream::SocketContextFactory {
private:
    core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
        return new EchoServerContext(socketConnection);
    }
};
```

#### Echo-Client ContextFactory

The `create()` method of our `EchoClientContextFactory` returns the `EchoClientContext` whose implementation is also presented in the [SocketContexts](#SocketContexts) section below.

``` c++
class EchoClientContextFactory : public core::socket::stream::SocketContextFactory {
private:
    core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
        return new EchoClientContext(socketConnection);
    }  
};
```

That\'s easy, isn\'t it?

### SocketContexts

It is also not difficult to implement the SocketContext classes for the server and the client.

-   Remember, we need to derive from the base class `core::socket::stream::SocketContext`.
-   And also remember the required functionality: The server shall reflect the received data back to the client!
-   And at last remember that the class  `core::socket::stream::SocketContext` needs the `core::socket::stream::SocketConnection`  to handle the physical data exchange.
    Thus, we have to pass the pointer to the SocketConnection to the constructor of the base `core::socket::stream::SocketContext`  class.

The base class `core::socket::stream::SocketContext` provides some virtual methods which can be overridden in an concrete SocketContext class. These methods will be called by the framework automaticaly.

#### Echo-Server Context

For our echo server application it would be sufficient to override the `onReceivedFromPeer()` method only. This method is called by the framework in case some data have already been received from the client. Nevertheless, for more information of what is going on in behind the methods `onConnected` and `onDisconnected` are overridden also.

In the `onReceivedFromPeer()` method we can fetch data already received by SNode.C by using the `readFromPeer()` method provided by the `core::socket::stream::SocketContext` class.

Sending data to the client is done using the method `sendToPeer()` which is also provided by the `core::socket::stream::SocketContext` class.

``` c++
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

Like in the `EchoServerContext` `readFromPeer()` and `sendToPeer()` is used in the `onReceivedFromPeer()` method. In addition `sendToPeer()` is also used in the `onConnected()` method to initiate the ping-pong data exchange.

``` c++
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

Now we can put all together and implement the server and client main applications. Here nameless instances are created - thus we will get no command line arguments automatically.

Note the use of our previously implemented `EchoServerContextFactory` and `EchoClientContextFactory` as template arguments.

At the very beginning SNode.C must be initialized by calling `core::SNodeC::init(argc, argv)`. And at the end of the main applications the event-loop of SNode.C is started by calling `core::SNodeC::start()`.

#### Echo-Server Main Application

The server instance `echoServer` must be activated by calling `echoServer.listen()`.

SNode.C provides a view overloaded `listen()` methods whose arguments vary depending on the network layer (IPv4, IPv6, RFCOM, L2CAP, or unix domain sockets) used. Though, every `listen()` method expects a lambda function as last argument. Here we use IPv4 and the `listen()` method which expects a port number as argument.

If we would have created a named server instance than a special `listen()` method which only expects the lambda function as argument can be used. In that case the configuration of this named instance would be done using command line arguments and/or a configuration file.

``` c++
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

The client instance `echoClient` must connect to the server by calling `echoClient.connect()`.

Equivalent to the server instance a client instance provides a view overloaded `connect()` methods whose arguments also vary depending on the network layer used. Here it is expected that the server runs on the same machine as the client.

If we would have created a named client instance than a special `connect()` method which only expects the lambda function can be used. In that case the configuration of this named instance would be done using command line arguments and/or a configuration file.

``` c++
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

The echo application shows the typical architecture of servers and clients implemented using SNode.C.

- The user needs to provide the application protocol layer by implementing the classe

  - SocketContextFactory and
  - SocketContext

  which need be be derived from the base classes

  - `core::socket::stream::SocketContextFactory`

  -   `core::socket::stream::SocketContext`

- The framework provides

  -   ready to use server and client template classes for each network/transport layer combination.

# Installation

SNode.C depends on some external libraries. Some of these libraries are directly included in the framework.

The only version-critical dependencies are the C++ compilers. Either the GCC or Clang can be used but they need to be of a relatively up to date version because SNode.C uses some new C++20 features internally.

## Minimum required Compiler Versions

- GCC 10.2
- Clang 11.0

## Supported Systems and Hardware

The main development of SNode.C takes place on an debian style linux system. Though, it should compile cleanly on every linux system provided that all required tools and libraries are installed.

SNode.C is known to compile and run successfull on

-   x86 architectures
    -   Tested on HP ZBook 15 G8
-   Arm architectures (32 and 64 bit)
    -   Tested on Raspberry Pi

## Tools

### Required

-   git ([<https://git-scm.com/>](https://git-scm.com/))
-   cmake ([<https://cmake.org/>](https://cmake.org/))
-   make ([<https://www.gnu.org/software/make/#:~:text=GNU%20Make%20is%20a%20tool,compute%20it%20from%20other%20files>.](https://www.gnu.org/software/make/#:~:text=GNU%20Make%20is%20a%20tool,compute%20it%20from%20other%20files.)) or
-   ninja ([<https://ninja-build.org/>](https://ninja-build.org/))
-   g++ ([<https://gcc.gnu.org/>](https://gcc.gnu.org/)) or
-   clang ([<https://clang.llvm.org/>](https://clang.llvm.org/))
-   pkg-config
    ([<https://www.freedesktop.org/wiki/Software/pkg-config/>](https://www.freedesktop.org/wiki/Software/pkg-config/))

### Optional

-   iwyu ([<https://include-what-you-use.org/>](https://include-what-you-use.org/))
-   clang-format ([<https://clang.llvm.org/docs/ClangFormat.html>](https://clang.llvm.org/docs/ClangFormat.html))
-   cmake-format ([<https://cmake-format.readthedocs.io/en/latest/cmake-format.html>](https://cmake-format.readthedocs.io/en/latest/cmake-format.html))
-   doxygen ([<https://www.doxygen.nl/>](https://www.doxygen.nl/))

## Libraries

### Required

-   Easylogging ([<https://github.com/amrayn/easyloggingpp>](https://github.com/amrayn/easyloggingpp))
-   OpenSSL ([<https://www.openssl.org/>](https://www.openssl.org/))
-   Nlohmann-JSON ([<https://json.nlohmann.me/>](https://json.nlohmann.me/))

### Optional

-   Bluez development files ([<http://www.bluez.org/>](http://www.bluez.org/))
-   LibMagic development files ([<https://www.darwinsys.com/file/>](https://www.darwinsys.com/file/))
-   MariaDB client development files ([<https://mariadb.org/>](https://mariadb.org/))

### In-Framework

This libraries are already integrated directly in SNode.C. Thus they need not be installed by hand

-   CLI11 ([<https://github.com/CLIUtils/CLI11>](https://github.com/CLIUtils/CLI11))

## Installation on Debian Style Systems (x86, Arm)

### Dependencies

To install all dependencies on debian style systems just run

``` sh
sudo apt update
sudo apt install git cmake make ninja-build g++ clang pkg-config
sudo apt install iwyu clang-format cmake-format doxygen
sudo apt install libeasyloggingpp-dev libssl-dev nlohmann-json3-dev
sudo apt install libbluetooth-dev libmagic-dev libmariadb-dev
```

### SNode.C

After installing all dependencies SNode.C can be cloned from github and compiled. 

This is strait forward:

``` sh
mkdir snode.c
cd snode.c
git clone https://github.com/VolkerChristian/snode.c.git
mkdir build
cd build
cmake ../snode.c
make
sudo make install
```

As SNode.C uses C++ templates a lot the compilation process will take some time. At least on a Raspberry Pi you can go for a coffee - it will take up to one and a half hour (on a Raspberry Pi 3) if just one core is activated for compilation.

It is a good idea to utilize all processor cores and threads for compilation. Thus e.g. on a Raspberry Pi append `-j4` to the `make`  or `ninja` command.

# Design Decisions and Features

-   Easy to use
-   Clear and clean architecture
-   Object orientated
-   Single-threaded
-   Single-tasking
-   Event driven
-   Layer based
-   Modular
-   Support for single shot and interval timer
-   For named server and client instances automated command line argument production and configuration file support

## Network Layer

SNode.C currently supports five different network layer protocols.

-   Internet Protocol version 4 (IPv4)
-   Internet Protocol version 6 (IPv6)
-   Unix Domain Sockets
-   Bluetooth Radio Frequency Communication (RFCOMM)
-   Bluetooth Logical Link Control and Adaptation Protocol (L2CAP)

## Transport Layer

Currently only connection-oriented protocols (SOCK_STREAM) for all supported network layer protocols are supported (for IPv4 and IPv6 this means TCP).

-   Every transport layer protocol provides a common base API which makes it very easy to create servers and clients for all different network layers supported.
-   New application protocols can be connected to the transport layer very easily by just implementing a SocketFactory and a SocketContext class.
-   Transparently offers SSL/TLS encryption provided by OpenSSL for each supportet network protocol and thus, also for all application level protocols.
    -   Support of X.509 certificates.
    -   Server Name Indication (SNI) is supported (useful for e.g. virtual web servers).

## Application Layer

In-framework support currently exist for the application level protocols

-   HTTP/1.1
-   WebSocket version 13
-   MQTT version 3.1.1 (version 5.0 is in preparation)
-   MQTT via WebSockets
-   High-Level Web API layer with JSON support very similar to the API of node.js/express.

# Example Applications

## HTTP/S Web-Server for Static HTML-Pages

This application uses the high-level web API which is very similar to the API of node.js/express. The `StaticMiddleware` is used to deliver the static HTML-pages.

The use of X.509 certificates for encrypted communication is demonstrated also.

``` cpp
#include <express/legacy/in/WebApp.h>
#include <express/tls/in/WebApp.h>
#include <express/middleware/StaticMiddleware.h>
#include <log/Logger.h>
#include <utils/Config.h>

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--web-root", "Root directory of the web site", "[path]");

    express::WebApp::init(argc, argv);
    
    using LegacyWebApp = express::legacy::in::WebApp;
    using LegacySocketAddress = LegacyWebApp::SocketAddress;

    LegacyWebApp legacyApp;
    legacyApp.getConfig().setReuseAddress();

    legacyApp.use(express::middleware::StaticMiddleware(utils::Config::get_string_option_value("--web-root")));

    legacyApp.listen(8080, [](const LegacySocketAddress& socketAddress, int errnum) {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "snode.c listening on " << socketAddress.toString();
        }
    });

    using TLSWebApp = express::tls::in::WebApp;
    using TLSSocketAddress = TLSWebApp::SocketAddress;

    TLSWebApp tlsApp;
    
    tlsApp.getConfig().setReuseAddress();

    tlsApp.getConfig().setCertChain("<path to X.509 certificate chain>");
    tlsApp.getConfig().setCertKey("<path to X.509 certificate key>");
    tlsApp.getConfig().setCertKeyPassword("<certificate key password>");

    tlsApp.use(express::middleware::StaticMiddleware(utils::Config::get_string_option_value("--web-root")));

    tlsApp.listen(8088, [](const TLSSocketAddress& socketAddress, int errnum) {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "snode.c listening on " << socketAddress.toString();
        }
    });

    return express::WebApp::start();
}
```

## Receive Data via HTTP-Post Request

The high-level web API provides the methods `get()`, `post()`, `put()`, etc like node.js/express.

``` cpp
#include <express/legacy/in/WebApp.h>
#include <express/tls/in/WebApp.h>
#include <log/Logger.h>

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);

    using LegacyWebApp = express::legacy::in::WebApp;
    using LegacySocketAddress = LegacyWebApp::SocketAddress;

    LegacyWebApp legacyApp;
    legacyApp.getConfig().setReuseAddress();

    // The macro APPLICATION(req, res) expands to (express::Request& req, express::Response& res)
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
                 "        <h1>File-Upload with input type=\"file\"</h1>"
                 "        <main>"
                 "            <h2>Send us something fancy!</h2>"
                 "            <form method=\"post\" enctype=\"multipart/form-data\">"
                 "                <label> Select a text file (*.txt, *.html etc.) from your computer."
                 "                    <input name=\"datei\" type=\"file\" size=\"50\" accept=\"text/*\">"
                 "                </label>"
                 "                <button>… and off we go!</button>"
                 "            </form>"
                 "        </main>"
                 "    </body>"
                 "</html>");
    });

    legacyApp.post("/", [] APPLICATION(req, res) {
        req.body.push_back(0);

        res.send("<html>"
                 "    <body>"
                 "        <h1>Thank you, we received your file!</h1>"
                 "        <h2>Content:</h2>"
                 "<pre>" +
                 std::string(reinterpret_cast<char*>(req.body.data())) +
                 "</pre>"
                 "    </body>"
                 "</html>");
    });

    legacyApp.listen(8080, [](const LegacySocketAddress& socketAddress, int errnum) -> void {
        if (errnum != 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "LegacyWebApp listening on " << socketAddress.toString();
        }
    });

    using TLSWebApp = express::tls::in::WebApp;
    using TLSSocketAddress = TLSWebApp::SocketAddress;

    TLSWebApp tlsApp;
    
    tlsApp.getConfig().setReuseAddress();

    tlsApp.getConfig().setCertChain("<path to X.509 certificate chain>");
    tlsApp.getConfig().setCertKey("<path to X.509 certificate key>");
    tlsApp.getConfig().setCertKeyPassword("<certificate key password>");

    tlsApp.use(legacyApp);

    tlsApp.listen(8088, [](const TLSSocketAddress& socketAddress, int errnum) -> void {
        if (errnum != 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "TLSWebApp listening on " << socketAddress.toString();
        }
    });

    return express::WebApp::start();
}
```

## Extract Server and Client Information (host name, IP, port, SSL/TLS information)

``` cpp
To be documented soon
```

## Using Regular Expressions in Routes

``` cpp
To be documented soon
```



<!-- <p align="center"><img src="docs/assets/README/015_architecture.svg" alt="015_architecture" style="zoom:200%;" style="display: block; margin: 0 auto"/>
</p> -->

