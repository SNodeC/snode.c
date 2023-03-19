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
      * [Main Applications for Server and Client](#main-applications-for-server-and-client)
         * [Echo-Server Main Application](#echo-server-main-application)
         * [Echo-Client Main Application](#echo-client-main-application)
      * [CMakeLists.txt file for Building and Installing our <em>echoserver</em> and <em>echoclient</em>](#cmakeliststxt-file-for-building-and-installing-our-echoserver-and-echoclient)
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
   * [Installation on Debian Style Systems (x86-64, Arm)](#installation-on-debian-style-systems-x86-64-arm)
      * [Dependencies](#dependencies)
      * [SNode.C](#snodec)
* [Design Decisions and Features](#design-decisions-and-features)
   * [Network Layer](#network-layer)
   * [Transport Layer](#transport-layer)
   * [Application Layer](#application-layer)
* [Existing Server- and Client-Classes](#existing-server--and-client-classes)
   * [SocketAddress](#socketaddress)
      * [SocketAddress Classes](#socketaddress-classes)
      * [SocketAddress Header Files](#socketaddress-header-files)
      * [SocketAddress Constructors](#socketaddress-constructors)
   * [Server](#server)
      * [SocketServer Classes](#socketserver-classes)
      * [SocketServer Header Files](#socketserver-header-files)
      * [Listen Methods](#listen-methods)
         * [Common listen() Methods](#common-listen-methods)
         * [Specific listen() Methods](#specific-listen-methods)
            * [IPv4 specific listen() Methods](#ipv4-specific-listen-methods)
            * [IPv6 specific listen() Methods](#ipv6-specific-listen-methods)
            * [Unix Domain Socket specific listen() Methods](#unix-domain-socket-specific-listen-methods)
            * [Bluetooth RFCOMM specific listen() Methods](#bluetooth-rfcomm-specific-listen-methods)
            * [Bluetooth L2CAP specific listen() Methods](#bluetooth-l2cap-specific-listen-methods)
   * [Client](#client)
      * [SocketClient Classes](#socketclient-classes)
      * [SocketClient Header Files](#socketclient-header-files)
      * [Connect Methods](#connect-methods)
         * [Common connect() Methods](#common-connect-methods)
         * [Specific connect() Methods](#specific-connect-methods)
            * [IPv4 specific connect() Methods](#ipv4-specific-connect-methods)
            * [IPv6 specific connect() Methods](#ipv6-specific-connect-methods)
            * [Unix Domain Socket specific connect() Methods](#unix-domain-socket-specific-connect-methods)
            * [Bluetooth RFCOMM specific connect() Methods](#bluetooth-rfcomm-specific-connect-methods)
            * [Bluetooth L2CAP specific connect() Methods](#bluetooth-l2cap-specific-connect-methods)
* [Configuration](#configuration)
   * [Configuration using the C++ API](#configuration-using-the-c-api)
      * [List of all Configuration Items](#list-of-all-configuration-items)
   * [Configuration via the Command Line](#configuration-via-the-command-line)
      * [Introduction to the Command Line Interface using the EchoServer from above](#introduction-to-the-command-line-interface-using-the-echoserver-from-above)
      * [Anatomy of the Command Line Interface](#anatomy-of-the-command-line-interface)
      * [Using the Parameterless listen() Methods when no Configuration File exists](#using-the-parameterless-listen-methods-when-no-configuration-file-exists)
      * [Command Line Configuration of the Client Instance EchoClient](#command-line-configuration-of-the-client-instance-echoclient)
   * [SSL/TLS-Configuration](#ssltls-configuration)
* [Highlevel WEB-API a'la Node.JS-Express](#highlevel-web-api-ala-nodejs-express)
* [Websockets](#websockets)
* [Example Applications](#example-applications)
   * [HTTP/S Web-Server for Static HTML-Pages](#https-web-server-for-static-html-pages)
   * [Receive Data via HTTP-Post Request](#receive-data-via-http-post-request)
   * [Extract Server and Client Information (host name, IP, port, SSL/TLS information)](#extract-server-and-client-information-host-name-ip-port-ssltls-information)
   * [Using Regular Expressions in Routes](#using-regular-expressions-in-routes)

<!-- Added by: runner, at: Sat Mar 18 21:03:14 UTC 2023 -->

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

Let\'s have a look at how these three components are related to each other by implementing a simple networking application.

## An "Echo" Application

Imagine we want to create a very basic TCP (**stream**)/IPv4 (**in**) server/client pair which sends some plain text data unencrypted (**legacy**) to each other in a ping-pong way.

The client shall start sending text data to the server and the server shall reflect that data back to the client. The client receives this reflected data and sends it back again to the server. This data ping-pong shall last infinitely long.

The code of this demo application can be found on [github](https://github.com/VolkerChristian/echo).

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

Both *instance-classes* have a *default constructor* and a *constructor expecting an instance name* as argument. 

- When the default constructor is used to create the instance object this instance is called an *anonymous instance*.
- In contrast to a *named instance* if the constructors expecting a `std::string` is used for instance creation. 
- For named instances *command line arguments and configuration file entries are created automatically* to configure the instance.

A class `SocketContextFactory` is used for both instances as template argument. Such a `SocketContextFactory` is used internally by the `SocketServer` and the `SocketClient` for creating a concrete `SocketContext` object for each established connection. This `SocketContext` represents a concrete application protocol.

Thus, for our echo application we need to implement the application logic (application protocol) for server and client in classes derived from `core::socket::stream::SocketContext`, which is the base class of all connection-oriented (stream) application protocols, and factories derived from `core::socket::stream::SocketContextFactory`.

### SocketContextFactories

Let\'s focus on the SocketContextFactories for our server and client first.

All what needs to be done is to implement a pure virtual method `create()`witch expects a pointer to a `core::socket::stream::SocketConnection` as argument and returns a concrete application SocketContext.

The `core::socket::stream::SocketConnection` object involved is managed internally by SNode.C and represents the *physical connection* between the server and a client. This `core::socket::stream::SocketConnection` is used internally by the `core::socket::stream::SocketContext` to handle the physical data transfer between server and client.

#### Echo-Server ContextFactory

The `create()` method of our `EchoServerContextFactory` returns the `EchoServerContext` whose implementation is presented in the [SocketContexts](#SocketContexts) section below.

``` c++
#include "EchoServerContext.h"
#include <core/socket/stream/SocketContextFactory.h>

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
#include "EchoClientContext.h"
#include <core/socket/stream/SocketContextFactory.h>

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

-   Remember, the required functionality: The server shall reflect the received data back to the client!
-   And also remember we need to derive from the base class `core::socket::stream::SocketContext`.
-   And at last remember that the class  `core::socket::stream::SocketContext` needs the `core::socket::stream::SocketConnection`  to handle the physical data exchange. Thus, we have to pass the pointer to the SocketConnection to the constructor of the base `core::socket::stream::SocketContext`  class.

The base class `core::socket::stream::SocketContext` provides *some virtual methods* which can be overridden in an concrete SocketContext class. These methods will be *called by the framework automatically*.

#### Echo-Server Context

For our echo server application it would be sufficient to override the `onReceivedFromPeer()` method only. This method is called by the framework in case some data have already been received from the client. Nevertheless, for more information of what is going on in behind the methods `onConnected` and `onDisconnected` are overridden also.

In the `onReceivedFromPeer()` method we can fetch data already received by SNode.C by using the `readFromPeer()` method provided by the `core::socket::stream::SocketContext` class.

Sending data to the client is done using the method `sendToPeer()` which is also provided by the `core::socket::stream::SocketContext` class.

``` c++
#include <core/socket/SocketAddress.h>
#include <core/socket/stream/SocketConnection.h>
#include <core/socket/stream/SocketContext.h>
#include <iostream>
#include <string>

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
            std::cout << "Data to reflect: " << std::string(junk, junkLen);
            sendToPeer(junk, junkLen); // Reflect the received data back to the client.
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
#include <core/socket/SocketAddress.h>
#include <core/socket/stream/SocketConnection.h>
#include <core/socket/stream/SocketContext.h>
#include <iostream>
#include <string>

class EchoClientContext : public core::socket::stream::SocketContext {
public:
    explicit EchoClientContext(core::socket::stream::SocketConnection* socketConnection)
        : core::socket::stream::SocketContext(socketConnection) {
    }

private:
    void onConnected() override { // Called in case a connection has been established successfully.
        std::cout << "Echo connected to " << socketConnection->getRemoteAddress().toString() << std::endl;

        std::cout << "Initiating data exchange" << std::endl;
        sendToPeer("Hello peer! It's nice talking to you!!!\n"); // Initiate the ping-pong data exchange.
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
            std::cout << "Data to reflect: " << std::string(junk, junkLen);
            sendToPeer(junk, junkLen); // Reflect the received data back to the server.
                                       // Out of memory is the only error which can occure here.
        }

        return junkLen; // Return the amount of data processed to the framework.
    }
};
```

### Main Applications for Server and Client

Now we can put all together and implement the server and client main applications. Here *anonymous instances* are used, thus we will not get command line arguments automatically.

Note the use of our previously implemented `EchoServerContextFactory` and `EchoClientContextFactory` as template arguments.

At the very beginning SNode.C must be *initialized* by calling `core::SNodeC::init(argc, argv)`. And at the end of the main applications the *event-loop* of SNode.C is started by calling `core::SNodeC::start()`.

#### Echo-Server Main Application

The server instance `echoServer` must be *activated* by calling `echoServer.listen()`.

SNode.C provides a view overloaded `listen()` methods whose arguments vary depending on the network layer (IPv4, IPv6, RFCOM, L2CAP, or unix domain sockets) used. Though, every `listen()` method expects a lambda function as last argument. Here we use IPv4 and the `listen()` method which expects a port number as argument.

If we would have created a named server instance than a special `listen()` method which only expects the lambda function as argument can be used. In that case the configuration of this named instance would be done using command line arguments and/or a configuration file.

``` c++
#include "EchoServerContextFactory.h"
#include <core/SNodeC.h>
#include <iostream>
#include <net/in/stream/legacy/SocketServer.h>
#include <string.h>
#include <string>

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv); // Initialize the framework.
                                    // Configure logging, create command line
                                    // arguments for named instances.

    using EchoServer = net::in::stream::legacy::SocketServer<EchoServerContextFactory>; // Simplify data type
                                                                                        // Note the use of our implemented
                                                                                        // EchoServerContextFactory as
                                                                                        // template argument
    using SocketAddress = EchoServer::SocketAddress;                                    // Simplify data type

    EchoServer echoServer; // Create anonymous server instance

    echoServer.listen(8001, [](const SocketAddress& socketAddress, int err) -> void { // Listen on port 8001 on all interfaces
        if (err == 0) {
            std::cout << "Success: Echo server listening on " << socketAddress.toString() << std::endl;
        } else {
            std::cout << "Error: Echo server listening on " << socketAddress.toString() << ": " << strerror(err) << std::endl;
        }
    });

    return core::SNodeC::start(); // Start the event loop, daemonize if requested.
}
```

#### Echo-Client Main Application

The client instance `echoClient` must *connect* to the server by calling `echoClient.connect()`.

Equivalent to the server instance a client instance provides a view overloaded `connect()` methods whose arguments also vary depending on the network layer used. Here it is assumed that we talk to an IPv4 server which runs on the same machine (localhost) as the client. Thus we pass the hostname "localhost" and port number 8001 to the `connect()` method.

If we would have created a named client instance than a special `connect()` method which only expects the lambda function can be used. In that case the configuration of this named instance would be done using command line arguments and/or a configuration file.

``` cpp
#include "EchoClientContextFactory.h"
#include <core/SNodeC.h>
#include <iostream>
#include <net/in/stream/legacy/SocketClient.h>
#include <string.h>
#include <string>

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv); // Initialize the framework.
                                    // Configure logging, create command line
                                    // arguments for named instances.

    using EchoClient = net::in::stream::legacy::SocketClient<EchoClientContextFactory>; // Simplify data type
                                                                                        // Note the use of our implemented
                                                                                        // EchoClientContextFactory as
                                                                                        // template argument
    using SocketAddress = EchoClient::SocketAddress;                                    // Simplify data type

    EchoClient echoClient; // Create anonymous client instance

    echoClient.connect("localhost", 8001, [](const SocketAddress& socketAddress,
                          int err) -> void { // Connect to server
                           if (err == 0) {
                               std::cout << "Success: Echo connected to " << socketAddress.toString() << std::endl;
                           } else {
                               std::cout << "Error: Echo client connected to " << socketAddress.toString() << ": " << strerror(err)
                                         << std::endl;
                           }
                       });

    return core::SNodeC::start(); // Start the event loop, daemonize if requested.
}
```

### CMakeLists.txt file for Building and Installing our *echoserver* and *echoclient*

```cmake
cmake_minimum_required(VERSION 3.3)

project(echo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(snodec COMPONENTS net-in-stream-legacy)

set(ECHOSERVER_CPP EchoServer.cpp)
set(ECHOSERVER_H EchoServerContextFactory.h EchoServerContext.h)

add_executable(echoserver ${ECHOSERVER_CPP} ${ECHOSERVER_H})
target_link_libraries(echoserver PRIVATE snodec::net-in-stream-legacy)

set(ECHOCLIENT_CPP EchoClient.cpp)
set(ECHOCLIENT_H EchoClientContextFactory.h EchoServerContext.h)

add_executable(echoclient ${ECHOCLIENT_CPP} ${ECHOCLIENT_H})
target_link_libraries(echoclient PRIVATE snodec::net-in-stream-legacy)

install(TARGETS echoserver echoclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
```

## Summary

The echo application shows the typical architecture of servers and clients using SNode.C.

- The user needs to provide the application protocol layer by implementing the classe

  - SocketContextFactory and
  - SocketContext

  which need be be derived from the base classes

  - [`core::socket::stream::SocketContextFactory`](https://volkerchristian.github.io/snode.c-doc/html/classcore_1_1socket_1_1stream_1_1_socket_context_factory.html)

  -   [`core::socket::stream::SocketContext`](https://volkerchristian.github.io/snode.c-doc/html/classcore_1_1socket_1_1stream_1_1_socket_context.html)

- The framework provides

  -   ready to use server and client template classes for each network/transport layer combination.

# Installation

SNode.C depends on some external libraries. Some of these libraries are directly included in the framework.

## Minimum required Compiler Versions

The only version-critical dependencies are the C++ compilers.

Either *GCC* or *clang* can be used but they need to be of an up to date version because SNode.C uses some new C++20 features internally.

- GCC 12.2
- Clang 13.0

## Supported Systems and Hardware

The main development of SNode.C takes place on an Debian style linux system. Though, it should compile cleanly on every linux system provided that all required tools and libraries are installed.

SNode.C is known to compile and run successfull on

-   x86-64 architecture
    -   Tested on HP ZBook 15 G8
-   Arm architecture (32 and 64 bit)
    -   Tested on Raspberry Pi

## Tools

### Required

-   git ([<https://git-scm.com/>](https://git-scm.com/))
-   cmake ([<https://cmake.org/>](https://cmake.org/))
-   make ([<https://www.gnu.org/software/make/>](https://www.gnu.org/software/make/)) or
-   ninja ([<https://ninja-build.org/>](https://ninja-build.org/))
-   g++ ([<https://gcc.gnu.org/>](https://gcc.gnu.org/)) or
-   clang ([<https://clang.llvm.org/>](https://clang.llvm.org/))
-   pkg-config
    ([<https://www.freedesktop.org/wiki/Software/pkg-config/>](https://www.freedesktop.org/wiki/Software/pkg-config/))

### Optional

-   iwyu ([<https://include-what-you-use.org/>](https://include-what-you-use.org/))
-   clang-format ([<https://clang.llvm.org/docs/ClangFormat.html>](https://clang.llvm.org/docs/ClangFormat.html))
-   cmake-format ([<https://cmake-format.readthedocs.io/>](https://cmake-format.readthedocs.io/))
-   doxygen ([<https://www.doxygen.nl/>](https://www.doxygen.nl/))

## Libraries

### Required

-   Easylogging development files ([<https://github.com/amrayn/easyloggingpp/>](https://github.com/amrayn/easyloggingpp/))
-   OpenSSL development files ([<https://www.openssl.org/>](https://www.openssl.org/))
-   Nlohmann-JSON development files([<https://json.nlohmann.me/>](https://json.nlohmann.me/))

### Optional

-   Bluez development files ([<http://www.bluez.org/>](http://www.bluez.org/))
-   LibMagic development files ([<https://www.darwinsys.com/file/>](https://www.darwinsys.com/file/))
-   MariaDB client development files ([<https://mariadb.org/>](https://mariadb.org/))

### In-Framework

This libraries are already integrated directly in SNode.C. Thus they need not be installed by hand

-   CLI11 ([<https://github.com/CLIUtils/CLI11/>](https://github.com/CLIUtils/CLI11/))

## Installation on Debian Style Systems (x86-64, Arm)

### Dependencies

To install all dependencies on Debian style systems just run

``` sh
sudo apt update
sudo apt install git cmake make ninja-build g++ clang pkg-config
sudo apt install iwyu clang-format cmake-format doxygen
sudo apt install libeasyloggingpp-dev libssl-dev nlohmann-json3-dev
sudo apt install libbluetooth-dev libmagic-dev libmariadb-dev
```

### SNode.C

After installing all dependencies SNode.C can be cloned from github, compiled, and installed.

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
sudo groupadd --system snodec
sudo usermod -a -G snodec root
sudo ldconfig
```

As SNode.C uses C++ templates a lot the compilation process will take some time. At least on a Raspberry Pi you can go for a coffee - it will take up to one and a half hour (on a Raspberry Pi 3 if just one core is activated for compilation).

It is a good idea to utilize all processor cores and threads for compilation. Thus e.g. on a Raspberry Pi append `-j4` to the `make`  or `ninja` command.

# Design Decisions and Features

-   Easy to use and extend
-   Clear and clean architecture
-   Object orientated
-   Single-threaded
-   Single-tasking
-   Event driven
-   Layer based
-   Modular
-   Support for single shot and interval timer
-   Automated command line argument production and configuration file support for named server and client instances
-   Sophisticated configuration system controlled either by code, command line, or configuration file
-   Daemonize server and client if requested

## Network Layer

SNode.C currently supports five different network layer protocols.

-   Internet Protocol version 4 (IPv4)
-   Internet Protocol version 6 (IPv6)
-   Unix Domain Sockets
-   Bluetooth Radio Frequency Communication (RFCOMM)
-   Bluetooth Logical Link Control and Adaptation Protocol (L2CAP)

## Transport Layer

Currently only connection-oriented protocols (SOCK_STREAM) for all supported network layer protocols are implemented (for IPv4 and IPv6 this means TCP).

-   Every transport layer protocol provides a common base API which makes it very easy to create servers and clients for all different network layers supported.
-   New application protocols can be connected to the transport layer very easily by just implementing a `SocketContextFactory` and a `SocketContext` class.
-   Transparently offers SSL/TLS encryption provided by OpenSSL for each supported transport layer protocol and thus, also for all application level protocols.
    -   Support of X.509 certificates.
    -   Server Name Indication (SNI) is supported (useful for e.g. virtual (web) servers).

## Application Layer

In-framework server and client support currently exist for the application level protocols

-   HTTP/1.1
-   WebSocket version 13
-   MQTT version 3.1.1 (version 5.0 is in preparation)
-   MQTT via WebSockets
-   High-Level Web API layer with JSON support very similar to the API of node.js/express.

As said above in the transport layer section, SSL/TLS encryption is provided for all of these application layer protocols.

# Existing Server- and Client-Classes

Before focusing explicitly on the Server- and Client-Classes a few common aspects for all network/transport-layer combinations needs to be known.

## SocketAddress

Every network layer provides its specific `SocketAddress` class. In typical scenarios you need not bother about these classes as they are managed internally by the framework.

Every *SocketServer* and *SocketClient* class has it's `SocketAddress` attached as nested data type. Thus, one can always get the correct `SocketAddress` type buy just

```cpp
using SocketAddress = <ConcreteServerOrClientType>::SocketAddress;
```

as can be seen in the Echo-Demo-Application above.

Nevertheless, for the sake of completeness, all implemented `SocketAddress` classes along with the header files they are declared in are listed below.

### SocketAddress Classes

| Network Layer       | `SocketAddress`                                              |
| ------------------- | ------------------------------------------------------------ |
| IPv4                | [`net::in::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1in_1_1_socket_address.html) |
| IPv6                | [`net::in6::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1in6_1_1_socket_address.html) |
| Unix Domain Sockets | [`net::un::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1un_1_1_socket_address.html) |
| Bluetooth RFCOMM    | [`net::rc::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1rc_1_1_socket_address.html) |
| Bluetooth L2CAP     | [`net::l2::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1l2_1_1_socket_address.html) |

### SocketAddress Header Files

| Network Layer       | `SocketAddress`           |
| ------------------- | ------------------------- |
| IPv4                | `net/in/SocketAddress.h`  |
| IPv6                | `net/in6/SocketAddress.h` |
| Unix Domain Sockets | `net/un/SocketAddress.h`  |
| Bluetooth RFCOMM    | `net/rc/SocketAddress.h`  |
| Bluetooth L2CAP     | `net/l2/SocketAddress.h`  |

Each `SocketAddress` class provides it's very specific set of constructors.

### SocketAddress Constructors

The default constructors of all `SocketAddress` classes creates wild-card `SocketAddress` objects. For a `SocketClient` for exampe, which uses such a wild-card `SocketAddress` as *local address* the operating system chooses a valid `sockaddr` structure automatically.

| SocketAddress                                                | Constructors                                                 |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| [`net::in::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1in_1_1_socket_address.html) | `SocketAddress()`<br/>`SocketAddress(const std::string& ipOrHostname)`<br/>`SocketAddress(const std::string& ipOrHostname, uint16_t port)`<br/>`SocketAddress(uint16_t port)` |
| [`net::in6::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1in6_1_1_socket_address.html) | `SocketAddress()`<br/>`SocketAddress(const std::string& ipOrHostname)`<br/>`SocketAddress(const std::string& ipOrHostname, uint16_t port)`<br/>`SocketAddress(uint16_t port)` |
| [`net::un::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1un_1_1_socket_address.html) | `SocketAddress()`<br/>`SocketAddress(const std::string& sunPath)` |
| [`net::rc::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1rc_1_1_socket_address.html) | `SocketAddress()`<br/>`SocketAddress(const std::string& btAddress)`<br/>`SocketAddress(const std::string& btAddress, uint8_t channel)`<br/>`SocketAddress(uint8_t channel)` |
| [`net::l2::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1l2_1_1_socket_address.html) | `SocketAddress()`<br/>`SocketAddress(const std::string& btAddress)`<br/>`SocketAddress(const std::string& btAddress, uint16_t psm)`<br/>`SocketAddress(uint16_t psm)` |


## Server

### SocketServer Classes

| Network Layer       | Legacy Connection                                            | SSL/TLS Connection                                           |
| ------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| IPv4                | [`net::in::stream::legacy::SocketServer`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in_1_1stream_1_1legacy.html) | [`net::in::stream::tls::SocketServer`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in_1_1stream_1_1tls.html) |
| IPv6                | [`net::in6::stream::legacy::SocketServer`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in6_1_1stream_1_1legacy.html) | [`net::in6::stream::tls::SocketServer`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in6_1_1stream_1_1legacy.html) |
| Unix Domain Sockets | [`net::un::stream::legacy::SocketServer`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1un_1_1stream_1_1legacy.html) | [`net::un::stream::tls::SocketServer`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1un_1_1stream_1_1tls.html) |
| Bluetooth RFCOMM    | [`net::rc::stream::legacy::SocketServer`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1rc_1_1stream_1_1legacy.html) | [`net::rc::stream::tls::SocketServer`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1rc_1_1stream_1_1tls.html) |
| Bluetooth L2CAP     | [`net::l2::stream::legacy::SocketServer`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1l2_1_1stream_1_1legacy.html) | [`net::l2::stream::tls::SocketServer`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1l2_1_1stream_1_1tls.html) |

### SocketServer Header Files


| Network Layer       | Legacy Connection                      | SSL/TLS Connection                  |
| ------------------- | -------------------------------------- | ----------------------------------- |
| IPv4                | `net/in/stream/legacy/SocketServer.h`  | `net/in/stream/tls/SocketServer.h`  |
| IPv6                | `net/in6/stream/legacy/SocketServer.h` | `net/in6/stream/tls/SocketServer.h` |
| Unix Domain Sockets | `net/un/stream/legacy/SocketServer.h`  | `net/un/stream/tls/SocketServer.h`  |
| Bluetooth RFCOMM    | `net/rc/stream/legacy/SocketServer.h`  | `net/rc/stream/tls/SocketServer.h`  |
| Bluetooth L2CAP     | `net/l2/stream/legacy/SocketServer.h`  | `net/l2/stream/tls/SocketServer.h`  |

### Listen Methods

As already mentioned above, each `SocketServer` class provides its own specific set of `listen()` methods. But some `listen()` methods are common to all `SocketServer` classes.

#### Common `listen()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const <ConcreteServerOrClientType>::SocketAddress&, int)>;
```

| `listen()`Method Type                                        | `listen()` Methods common to all SocketServer Classes        |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| Listen without parameter[^1]                                 | `void listen(StatusFunction& onError)`                       |
| Listen expecting a `SocketAddress` as argument               | `void listen(const SocketAddress& localAddress, StatusFunction& onError)` |
| Listen expecting a `SocketAddress` and a backlog as argument | `void listen(const SocketAddress& localAddress, int backlog, StatusFunction& onError)` |

[^1]: "Without parameter" is not completely right because every `listen()` method expects a reference to a `std::function` for status processing (error or success) as argument.

#### Specific `listen()` Methods

##### IPv4 specific `listen()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::in::SocketAddress&, int)>;
```

For the IPv4/SOCK_STREAM combination exist four specific `listen()` methods.

| `listen()` Methods                                           |
| ------------------------------------------------------------ |
| `void listen(uint16_t port, StatusFunction& onError)`        |
| `void listen(uint16_t port, int backlog, StatusFunction& onError)` |
| `void listen(const std::string& ipOrHostname, int backlog, StatusFunction& onError)` |
| `void listen(const std::string& ipOrHostname, uint16_t port, int backlog, StatusFunction& onError)` |

##### IPv6 specific `listen()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::in6::SocketAddress&, int)>;
```

For the IPv6/SOCK_STREAM combination exist four specific `listen()` methods.

| `listen()` Methods                                           |
| ------------------------------------------------------------ |
| `void listen(uint16_t port, StatusFunction& onError)`        |
| `void listen(uint16_t port, int backlog, StatusFunction& onError)` |
| `void listen(const std::string& ipOrHostname, int backlog, StatusFunction& onError)` |
| `void listen(const std::string& ipOrHostname, uint16_t port, int backlog, StatusFunction& onError)` |

##### Unix Domain Socket specific `listen()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::un::SocketAddress&, int)>;
```

For the Unix Domain Socket/SOCK_STREAM combination exist two specific `listen()` methods.

| `listen()` Methods                                           |
| ------------------------------------------------------------ |
| `void listen(const std::string& sunPath, StatusFunction& onError)` |
| `void listen(const std::string& sunPath, int backlog, StatusFunction& onError)` |

##### Bluetooth RFCOMM specific `listen()` Methods

IPv4 The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::rc::SocketAddress&, int)>;
```

For the RFCOMM/SOCK_STREAM combination exist four specific `listen()` methods.

| `listen()` Methods                                           |
| ------------------------------------------------------------ |
| `void listen(uint8_t channel, StatusFunction& onError)`      |
| `void listen(uint8_t channel, int backlog, StatusFunction& onError)` |
| `void listen(const std::string& btAddress, int backlog, StatusFunction& onError)` |
| `void listen(const std::string& btAddress, uint8_t channel, int backlog, StatusFunction& onError)` |

##### Bluetooth L2CAP specific `listen()` Methods

IPv4 The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::l2::SocketAddress&, int)>;
```

For the L2CAP/SOCK_STREAM combination exist four specific `listen()` methods.

| `listen()` Methods                                           |
| ------------------------------------------------------------ |
| `void listen(uint16_t psm, StatusFunction& onError)`         |
| `void listen(uint16_t psm, int backlog, StatusFunction& onError)` |
| `void listen(const std::string& btAddress, int backlog, StatusFunction& onError)` |
| `void listen(const std::string& btAddress, uint16_t psm, int backlog, StatusFunction& onError)` |

## Client

### SocketClient Classes


| Network Layer       | Legacy Connection                                            | SSL/TLS Connection                                           |
| ------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| IPv4                | [`net::in::stream::legacy::SocketClient`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in_1_1stream_1_1legacy.html) | [`net::in::stream::tls::SocketClient`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in_1_1stream_1_1tls.html) |
| IPv6                | [`net::in6::stream::legacy::SocketClient`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in6_1_1stream_1_1legacy.html) | [`net::in6::stream::tls::SocketClient`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in6_1_1stream_1_1tls.html) |
| Unix Domain Sockets | [`net::un::stream::legacy::SocketClient`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1un_1_1stream_1_1legacy.html) | [`net::un::stream::tls::SocketClient`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1un_1_1stream_1_1tls.html) |
| Bluetooth RFCOMM    | [`net::rc::stream::legacy::SocketClient`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1rc_1_1stream_1_1legacy.html) | [`net::rc::stream::tls::SocketClient`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1rc_1_1stream_1_1tls.html) |
| Bluetooth L2CAP     | [`net::l2::stream::legacy::SocketClient`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1l2_1_1stream_1_1legacy.html) | [`net::l2::stream::tls::SocketClient`](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1l2_1_1stream_1_1tls.html) |

### SocketClient Header Files


| Network Layer       | Legacy Connection                      | SSL/TLS Connection                  |
| ------------------- | -------------------------------------- | ----------------------------------- |
| IPv4                | `net/in/stream/legacy/SocketClient.h`  | `net/in/stream/tls/SocketClient.h`  |
| IPv6                | `net/in6/stream/legacy/SocketClient.h` | `net/in6/stream/tls/SocketClient.h` |
| Unix Domain Sockets | `net/un/stream/legacy/SocketClient.h`  | `net/un/stream/tls/SocketClient.h`  |
| Bluetooth RFCOMM    | `net/rc/stream/legacy/SocketClient.h`  | `net/rc/stream/tls/SocketClient.h`  |
| Bluetooth L2CAP     | `net/l2/stream/legacy/SocketClient.h`  | `net/l2/stream/tls/SocketClient.h`  |

### Connect Methods

As already mentioned above, each `SocketClient` class provides its own specific set of `connect()` methods. But some `connect()` methods are common to all `SocketClient` classes.

#### Common `connect()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const <ConcreteServerOrClientType>::SocketAddress&, int)>;
```

| `connect()`Method Type                             | `connect()` Methods common to all SocketServer Classes       |
| -------------------------------------------------- | ------------------------------------------------------------ |
| Connect without parameter[^2]                      | `void connect(StatusFunction& onError)`                      |
| Connect expecting a `SocketAddress` as argument    | `void connect(const SocketAddress& remoteAddress, StatusFunction& onError)` |
| Connect expecting two `SocketAddress`s as argument | `void connect(const SocketAddress& remoteAddress, const SocketAddress& localAddress, StatusFunction& onError)` |

[^2]: "Without parameter" is not completely right because every `connect()` method expects a `std::function` for status processing (error or success) as argument. 

#### Specific connect() Methods

##### IPv4 specific `connect()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::in::SocketAddress&, int)>;
```

For the IPv4/SOCK_STREAM combination exist three specific `connect()` methods.

| `connect()` Methods                                          |
| ------------------------------------------------------------ |
| `void connect(const std::string& ipOrHostname, uint16_t port, StatusFunction& onError)` |
| `void connect(const std::string& ipOrHostname, uint16_t port, const std::string& bindIpOrHostname, StatusFunction& onError)` |
| `void connect(const std::string& ipOrHostname, uint16_t port, const std::string& bindIpOrHostname, uint16_t bindPort, StatusFunction& onError)` |

##### IPv6 specific `connect()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::in6::SocketAddress&, int)>;
```

For the IPv6/SOCK_STREAM combination exist three specific `connect()` methods.

| `connect()` Methods                                          |
| ------------------------------------------------------------ |
| `void connect(const std::string& ipOrHostname, uint16_t port, StatusFunction& onError)` |
| `void connect(const std::string& ipOrHostname, uint16_t port, const std::string& bindIpOrHostname, StatusFunction& onError)` |
| `void connect(const std::string& ipOrHostname, uint16_t port, const std::string& bindIpOrHostname, uint16_t bindPort, StatusFunction& onError)` |

##### Unix Domain Socket specific `connect()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::un::SocketAddress&, int)>;
```

For the Unix Domain Socket/SOCK_STREAM combination exist two specific `connect()` methods.

| `connect()` Methods                                          |
| ------------------------------------------------------------ |
| `void connect(const std::string& sunPath, StatusFunction& onError)` |
| `void connect(const std::string& remoteSunPath, const std::string& localSunPath, StatusFunction& onError)` |

##### Bluetooth RFCOMM specific `connect()` Methods

IPv4 The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::rc::SocketAddress&, int)>;
```

For the RFCOMM/SOCK_STREAM combination exist three specific `connect()` methods.

| `connect()` Methods                                          |
| ------------------------------------------------------------ |
| `void connect(const std::string& btAddress, uint8_t channel, StatusFunction& onError)` |
| `void connect(const std::string& btAddress, uint8_t channel, StatusFunction& onError)` |
| `void connect(const std::string& btAddress, uint8_t channel, const std::string& localAddress, uint8_t bindChannel, StatusFunction& onError)` |

##### Bluetooth L2CAP specific `connect()` Methods

IPv4 The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::l2::SocketAddress&, int)>;
```

For the L2CAP/SOCK_STREAM combination exist three specific `connect()` methods.

| `connect()` Methods                                          |
| ------------------------------------------------------------ |
| `void connect(const std::string& btAddress, uint16_t psm, StatusFunction& onError)` |
| `void connect(const std::string& btAddress, uint16_t psm, const std::string& localAddress, StatusFunction& onError)` |
| `void connect(const std::string& btAddress, uint16_t psm, const std::string& localAddress, uint16_t bindPsm, StatusFunction& onError)` |

# Configuration

Each `SocketServer` and `SocketClient` instance needs to be configured before they can be started by the SNode.C event loop. Fore instance, a IPv4/TCP `SocketServer` needs to know at least the port number it should listen on, a `SocketClient` needs to now the host name or the IP address and the port number a server is listening on. And if an SSL/TLS instance is used certificates are necessary for successful encryption.

There are many more configuration items but lets focus on those mentioned above.

SNode.C provides three different ways to specify such configuration items. Nevertheless, internally all uses the same underlying configuration system, which is entirely based on the great [CLI11: Command line parser for C++11](https://github.com/CLIUtils/CLI11) library.

The configuration can either be done via

- the provided C++ API directly in the source code for anonymous and named instances
- command line arguments for named instances
- configuration files for named instances

## Configuration using the C++ API

Each anonymous and named `SocketServer` and `SocketClient` instance provide an configuration object which could be obtained by calling the method `getConfig()` on the instance which returns a reference to that configuration object.

For the `EchoServer` instance from the "Quick Starting Guide" section for example the configuration object can be obtained by just using

```cpp
EchoServer echoServer;
echoServer.getConfig();
```

Such a configuration object provides many methods to specify the individual configuration items.

Thus, to configure the port number for the `echoServer` instance the method `setPort(uint16_t port)` of the configuration object can be used

```cpp
EchoServer echoServer;
echoServer.getConfig().setPort(8001);
```

This is what the `listen()` method which expects a port number as argument does automatically.

Thus, if the port number is configured by using `setPort()` the `listen()` method which only takes a `std::function` as argument can be use and the `EchoServer` could also be started by

```cpp
EchoServer echoServer;
echoServer.getConfig().setPort(8001);
echoServer.listen([](const SocketAddress& socketAddress, int err) -> void { // Listen on port 8001 on all interfaces
	if (err == 0){
        std::cout << "Success: Echo server listening on " << socketAddress.toString() << std::endl
    } else {
        std::cout << "Error: Echo server listening on " << socketAddress.toString() << ": " << perror("") << std::endl;
    }
});
```

The same technique can be used to configure the  `EchoClient` instance. 

Though, because a `SocketClient` has two independent sets of IP-Addresses/host names and port numbers, one for the remote side and one for the local side, one need to be more specific in which of these addresses shall be configured. Here the remote address is configured explicitly.

```cpp
EchoServer echoClient;
echoClient.getConfig().Remote::setIpOrHostname("localhost");
echoClient.getConfig().Remote::setPort(8001);
echoClient.connect([](const SocketAddress& socketAddress, int err) -> void { // Listen on port 8001 on all interfaces
	if (err == 0){
        std::cout << "Success: Echo client connected to " << socketAddress.toString() << std::endl
    } else {
        std::cout << "Error: Echo client connected to " << socketAddress.toString() << ": " << perror("") << std::endl;
    }
});
```

Other configuration items can be configured in the very same way but for most option items sane default values are already predefined. 

### List of all Configuration Items

All `SocketServer` and `SocketClient` instances share some common [configuration options](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1config.html).

Network layer specific configuration options:

| Network Layer       | Address                                                      | Transport Layer                                              | Legacy                                                       | TLS                                                          |
| ------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| IPv4                | [Address configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in_1_1config.html) | [Transport layer configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in_1_1stream_1_1config.html) | [Legacy configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in_1_1stream_1_1legacy_1_1config.html) | [TLS configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in_1_1stream_1_1tls_1_1config.html) |
| IPv6                | [Address configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in6_1_1config.html) | [Transport layer configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in6_1_1stream_1_1config.html) | [Legacy configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in6_1_1stream_1_1legacy_1_1config.html) | [TLS configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1in6_1_1stream_1_1tls_1_1config.html) |
| Unix Domain Sockets | [Address configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1un_1_1config.html) | [Transport layer configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1un_1_1stream_1_1config.html) | [Legacy configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1un_1_1stream_1_1legacy_1_1config.html) | [TLS configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1un_1_1stream_1_1tls_1_1config.html) |
| Bluetooth RFCOMM    | [Address configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1rc_1_1config.html) | [Transport layer configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1rc_1_1stream_1_1config.html) | [Legacy configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1rc_1_1stream_1_1legacy_1_1config.html) | [TLS configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1rc_1_1stream_1_1tls_1_1config.html) |
| Bluetoot L2CAP      | [Address configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1l2_1_1config.html) | [Transport layer configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1l2_1_1stream_1_1config.html) | [Legacy configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1l2_1_1stream_1_1legacy_1_1config.html) | [TLS configuration](https://volkerchristian.github.io/snode.c-doc/html/namespacenet_1_1l2_1_1stream_1_1tls_1_1config.html) |

## Configuration via the Command Line

Each application gets a set of common command line options which control the behavior of the application in general. An overview of those option can be printed on screen by adding `--help` or `--help-all` on the command line. 

### Introduction to the Command Line Interface using the `EchoServer` from above

For instance 

```shell
command@line:~/> echoserver --help
```

leads to the help output

```shell
Configuration for Application 'echoserver'

Usage: echoserver [OPTIONS]

Options (non-persistent):
  -h,--help
       Print this help message and exit
  -a,--help-all
       Print this help message, expand instances and exit
  --version
       Display program version information and exit
  -s,--show-config
       Show current configuration and exit
  -w,--write-config [configfile]:NOT DIR [/home/<user>/.config/snode.c/echoserver.conf] 
       Write config file and exit
  --config-file configfile:NOT DIR [/home/<user>/.config/snode.c/echoserver.conf] 
       Read an config file
  --instance-map name=mapped_name 
       Instance name mapping used to make an instance known under an alias name also in a config file
  -k,--kill
       Kill running daemon
  --commandline
       Print a template command line showing required options only and exit
  --commandline-full
       Print a template command line showing all possible options and exit
  --commandline-configured
       Print a template command line showing all required and configured options and exit

Options (persistent):
  -l,--log-level level:INT in [0 - 6] [3] 
       Log level
  -v,--verbose-level level:INT in [0 - 10] [0] 
       Verbose level
  --log-file logfile:NOT DIR [/home/<user>/.local/log/snode.c/echoserver.log] 
       Logfile path
  --enforce-log-file={true,false} [false] 
       Enforce writing of logs to file for foreground applications
  -d{true},-f{false},--daemonize={true,false} [false] 
       Start application as daemon
  --user-name username [<user>]  Needs: --daemonize 
       Run daemon under specific user permissions
  --group-name groupname [<user>]  Needs: --daemonize 
       Run daemon under specific group permissions
```

Each named `SocketServer` and `SocketClient` instance get their specific set of command line options accessible by specifying the name of the instance on the command line.

Thus, for instance if the `EchoServer` instance is created using and instance name as argument to the server instance constructor like for example 

```cpp
EchoServer echoServer("echo"); // Create named server instance
```

 (try it yourself using the code from  [github](https://github.com/VolkerChristian/echo)), the output of the help screen changes slightly:

```shell
Configuration for Application 'echoserver'

Usage: echoserver [OPTIONS] [INSTANCE]

Options (non-persistent):
  -h,--help
       Print this help message and exit
  -a,--help-all
       Print this help message, expand instances and exit
  --version
       Display program version information and exit
  -s,--show-config
       Show current configuration and exit
  -w,--write-config [configfile]:NOT DIR [/home/<user>/.config/snode.c/echoserver.conf] 
       Write config file and exit
  --config-file configfile:NOT DIR [/home/<user>/.config/snode.c/echoserver.conf] 
       Read an config file
  --instance-map name=mapped_name 
       Instance name mapping used to make an instance known under an alias name also in a config file
  -k,--kill
       Kill running daemon
  --commandline
       Print a template command line showing required options only and exit
  --commandline-full
       Print a template command line showing all possible options and exit
  --commandline-configured
       Print a template command line showing all required and configured options and exit

Options (persistent):
  -l,--log-level level:INT in [0 - 6] [3] 
       Log level
  -v,--verbose-level level:INT in [0 - 10] [0] 
       Verbose level
  --log-file logfile:NOT DIR [/home/<user>/.local/log/snode.c/echoserver.log] 
       Logfile path
  --enforce-log-file={true,false} [false] 
       Enforce writing of logs to file for foreground applications
  -d{true},-f{false},--daemonize={true,false} [false] 
       Start application as daemon
  --user-name username [<user>]  Needs: --daemonize 
       Run daemon under specific user permissions
  --group-name groupname [<user>]  Needs: --daemonize 
       Run daemon under specific group permissions

Instances:
  echo Configuration for server instance 'echo'
```

Note that now the named instance *echo* now appears at the end of the help screen.

To get informations about what can be configured for the *echo* instance it is just needed to write

```shell
command@line:~/> echoserver echo --help
```

what prints the output

```shell
Configuration for server instance 'echo'

Usage: echoserver echo [OPTIONS] [SECTIONS]

Options (non-persistent):
  -h,--help
       Print this help message and exit
  --help-all
       Print this help message, expand sections and exit
  --commandline
       Print a template command line showing required options only and exit
  --commandline-full
       Print a template command line showing all possible options and exit
  --commandline-configured
       Print a template command line showing all required and configured options and exit

Options (persistent):
  --disable={true,false} [false] 
       Disable this instance

Sections:
  local
       Local side of connection for instance 'echo'
  connection
       Configuration of established connections for instance 'echo'
  socket
       Configuration of socket behaviour for instance 'echo'
  server
       Configuration of server socket for instance 'echo'
  cluster
       Configuration of clustering mode for instance 'echo'
```

on screen.

As one can see, there exists some sections for the instance *echo* each offering specific configuration items for specific instance behavior categories. Most important for a `SocketServer` instance is the section *local*,

```shell
command@line:~/> echoserver echo local --help
Local side of connection for instance 'echo'

Usage: echoserver echo local [OPTIONS]

Options (non-persistent):
  -h,--help
       Print this help message and exit

Options (persistent):
  --host hostname|IPv4:TEXT [0.0.0.0] 
       Host name or IPv4 address
  --port port:UINT in [0 - 65535] [8001] 
       Port number
```

which offer configuration options to configure the hostname or IP-Address and port number the physical server socket should be bound to. Note, that the default value of the port number is `[8001]`, what is this port number used to activate the `echo` instance:

```cpp
echoServer.listen(8001, [](const SocketAddress& socketAddress, int err) -> void { // Listen on port 8001 on all interfaces
    ...
});
```

This port number can now be overridden on the command line so, that the `echo` listens on port number `8080`:

```shell
command@line:~/> echoserver echo local --port 8080
2023-03-11 18:53:12 0000000000001: echo mode: STANDALONE
Success: Echo server listening on 0.0.0.0:8080
```

To make this overridden port number setting persistent a configuration file can be generated and stored automatically by appending `-w`

```shell
command@line:~/> echoserver echo local --port 8080 -w
Writing config file: /home/[user]/.config/snode.c/echoserver.conf
```

to the command line above. If *echoserver* is now started without command line arguments

```shell
command@line:~/> echoserver
2023-03-11 18:59:46 0000000000001: echo mode: STANDALONE
Success: Echo server listening on 0.0.0.0:8080
```

the in the configuration file stored port number `8080` is used instead of the port number `8001` used directly in the code.

All existing configuration options specified directly in the application code can be overridden on the command line and/or the configuration file in that way.

### Anatomy of the Command Line Interface

The command line interface follows a well defined structure, for example

```shell
command@line:~/> serverorclientexecutable instancename1 section1 --sec1-opt1 val111 --sec1-opt2 val12 section2 --sec2-opt1 val21 --sec2-opt2 val22 instancename2 section1  --sec1-opt1 val111 --sec1-opt2 val12 section2 --sec2-opt1 val21 --sec2-opt2 val22
```

if two instances with instance names *instancename1* and *instancename2* are present in the executable.

All section names following an instance name are treaded as sections modifying that instance as long as no other instance name is specified. In case a further instance name is given, than all sections following that second instance name are treaded as sections modifying that second instance.

One can switch between sections by just specifying a different section name.

### Using the Parameterless `listen()` Methods when no Configuration File exists

In case the parameterless `listen()` method is used for activating a server instance for example like

```cpp
EchoServer echoServer("echo"); // Create server instance

echoServer.listen([](const SocketAddress& socketAddress, int err) -> void { // Port on command line or in config file
    if (err == 0) {
        std::cout << "Success: Echo server listening on " << socketAddress.toString() << std::endl;
    } else {
        std::cout << "Error: Echo server listening on " << socketAddress.toString() << ": " << strerror(err) << std::endl;
    }
});
```

 and no configuration file exists, SNode.C notifies that at least a port number needs to be configured on the command line:

```shell
command@line:~/> echoserver
Command line error: echo is required

command@line:~/> echoserver echo
Command line error: echo requires local

command@line:~/> echoserver echo local
Command line error: local requires --port

command@line:~/> echoserver echo local --port
Command line error: Argument for --port: 1 required port:UINT in [0 - 65535] missing

command@line:~/> echoserver echo local --port 8080
2023-03-11 19:26:36 0000000000001: echo mode: STANDALONE
Success: Echo server listening on 0.0.0.0:8080
```

Again, this configuration can be made permanent by writing the configuration file by appending `-w` to the command line:

```shell
command@line:~/> echoserver echo local --port 8080 -w
Writing config file: /home/[user]/.config/snode.c/echoserver.conf

command@line:~/> echoserver
2023-03-11 19:29:18 0000000000001: echo mode: STANDALONE
Success: Echo server listening on 0.0.0.0:8080
```

### Command Line Configuration of the Client Instance `EchoClient`

A client instance is configured in the very same way. 

Lets have look at the case of the named `echoclient` 

```cpp
EchoClient echoClient("echo"); // Create named client instance
echoClient.connect([](const SocketAddress& socketAddress,
                      int err) -> void { // Connect to server
    if (err == 0) {
        std::cout << "Success: Echo connected to " << socketAddress.toString() << std::endl;
    } else {
        std::cout << "Error: Echo client connected to " << socketAddress.toString() << ": " << strerror(err) << std::endl;
    }
});
```

where the parameterless `connect()` method is used. A terminal session would look like:

```shell
command@line:~/> echoclient
Command line error: echo is required

command@line:~/> echoclient echo
Command line error: echo requires remote

command@line:~/> echoclient echo remote
Command line error: remote requires --port

command@line:~/> echoclient echo remote --port
Command line error: Argument for --port: 1 required port:UINT in [0 - 65535] missing

command@line:~/> echoclient echo remote --port 8080
Command line error: remote requires --host

command@line:~/> echoclient echo remote --port 8080 --host
Command line error: Argument for --host: 1 required hostname|IPv4:TEXT missing

command@line:~/> echoclient echo remote --port 8080 --host localhost
2023-03-11 19:40:00 0000000000002: OnConnect echo
2023-03-11 19:40:00 0000000000002:      Local: (127.0.0.1) localhost:37023
2023-03-11 19:40:00 0000000000002:      Peer:  (127.0.0.1) localhost:8080
2023-03-11 19:40:00 0000000000002: OnConnected echo
Echo connected to localhost:8080
Initiating data exchange
Success: Echo connected to localhost:8080
Data to reflect: Hello peer! It's nice talking to you!!!
Data to reflect: Hello peer! It's nice talking to you!!!
...
```

Again this configuration can be made permanent by writing the configuration file using `-w` on the command line:

```shell
command@line:~/> echoclient echo remote --port 8080 --host localhost -w
Writing config file: /home/[user]/.config/snode.c/echoclient.conf

command@line:~/> echoclient
2023-03-11 19:42:00 0000000000002: OnConnect echo
2023-03-11 19:42:00 0000000000002:      Local: (127.0.0.1) localhost:38276
2023-03-11 19:42:00 0000000000002:      Peer:  (127.0.0.1) localhost:8080
2023-03-11 19:42:00 0000000000002: OnConnected echo
Echo connected to localhost:8080
Initiating data exchange
Success: Echo connected to localhost:8080
Data to reflect: Hello peer! It's nice talking to you!!!
Data to reflect: Hello peer! It's nice talking to you!!!
...
```

## SSL/TLS-Configuration

To be written

# Highlevel WEB-API a'la Node.JS-Express

To be written

# Websockets

To be written

# Example Applications

## HTTP/S Web-Server for Static HTML-Pages

This application uses the high-level web API *express* which is very similar to the API of node.js/express. The `StaticMiddleware` is used to deliver the static HTML-pages.

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

    // The macro 
    //    APPLICATION(req, res)
    // expands to 
    //    ([[maybe_unused]] express::Request& (req), [[maybe_unused]] express::Response& (res)
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

