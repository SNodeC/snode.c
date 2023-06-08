# Simple NODE in C++ (SNode.C)

**Claim**: It has certainly never been easier to develop WEB and networking applications in C++ (Please refute me in case).

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
      * [SocketServer and SocketClient Instances](#socketserver-and-socketclient-instances)
      * [SocketContextFactories](#socketcontextfactories)
         * [Echo-Server SocketContextFactory](#echo-server-socketcontextfactory)
         * [Echo-Client SocketContextFactory](#echo-client-socketcontextfactory)
      * [SocketContexts](#socketcontexts)
         * [Echo-Server SocketContext](#echo-server-socketcontext)
         * [Echo-Client SocketContext](#echo-client-socketcontext)
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
   * [SocketConnection](#socketconnection)
      * [Most Important common SocketConnection Methods](#most-important-common-socketconnection-methods)
   * [Constructors of SocketServer and SocketClient Classes](#constructors-of-socketserver-and-socketclient-classes)
      * [Constructors of SocketServer Classes](#constructors-of-socketserver-classes)
      * [Constructors of SocketClient Classes](#constructors-of-socketclient-classes)
      * [Constructor Callbacks](#constructor-callbacks)
         * [The onConnect() Callback](#the-onconnect-callback)
         * [The onConnected() Callback](#the-onconnected-callback)
         * [The onDisconnected() Callback](#the-ondisconnected-callback)
      * [Attaching the Callbacks during Instance Creation](#attaching-the-callbacks-during-instance-creation)
      * [Attaching the Callbacks to SocketServer and SocketClient Instances](#attaching-the-callbacks-to-socketserver-and-socketclient-instances)
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
   * [Three different Options for Configuration](#three-different-options-for-configuration)
   * [Configuration using the C++ API](#configuration-using-the-c-api)
      * [List of all Configuration Items](#list-of-all-configuration-items)
   * [Configuration via the Command Line](#configuration-via-the-command-line)
      * [Introduction to the Command Line Interface using the EchoServer from above](#introduction-to-the-command-line-interface-using-the-echoserver-from-above)
         * [Instance Configuration](#instance-configuration)
         * [Sections](#sections)
      * [Anatomy of the Command Line Interface](#anatomy-of-the-command-line-interface)
      * [Using the Parameterless listen() Methods when no Configuration File exists](#using-the-parameterless-listen-methods-when-no-configuration-file-exists)
      * [Command Line Configuration of the Client Instance EchoClient](#command-line-configuration-of-the-client-instance-echoclient)
   * [Configuration via a Configuration File](#configuration-via-a-configuration-file)
      * [Default Name of a Configuration File](#default-name-of-a-configuration-file)
      * [Location of Configuration Files](#location-of-configuration-files)
   * [Important Configuration Sections](#important-configuration-sections)
      * [SSL/TLS Configuration (Section <em>tls</em>)](#ssltls-configuration-section-tls)
         * [SSL/TLS In-Code Configuration](#ssltls-in-code-configuration)
         * [SSL/TLS Command Line Configuration](#ssltls-command-line-configuration)
         * [Using SSL/TLS with Other Network Layers](#using-ssltls-with-other-network-layers)
      * [Socket Configuration (Section <em>socket</em>)](#socket-configuration-section-socket)
         * [Common <em>socket</em> Options for SocketServer and SocketClient Instances](#common-socket-options-for-socketserver-and-socketclient-instances)
         * [Specific <em>socket</em> Options for IPv4 and IPv6 SocketServer](#specific-socket-options-for-ipv4-and-ipv6-socketserver)
         * [Specific <em>socket</em> Options for IPv6 SocketServer and SocketClient](#specific-socket-options-for-ipv6-socketserver-and-socketclient)
* [Using more than one Instance in an Application](#using-more-than-one-instance-in-an-application)
* [Highlevel WEB-API a'la Node.JS-Express](#highlevel-web-api-ala-nodejs-express)
* [Websockets](#websockets)
* [Example Applications](#example-applications)
   * [HTTP/S Web-Server for Static HTML-Pages](#https-web-server-for-static-html-pages)
   * [Receive Data via HTTP-Post Request](#receive-data-via-http-post-request)
   * [Extract Server and Client Information (host name, IP, port, SSL/TLS information)](#extract-server-and-client-information-host-name-ip-port-ssltls-information)
   * [Using Regular Expressions in Routes](#using-regular-expressions-in-routes)

<!-- Added by: runner, at: Sun Apr 23 20:45:50 UTC 2023 -->

<!--te-->

# License

SNode.C is released under the **GNU Lesser General Public License, Version 3** ([<https://www.gnu.org/licenses/lgpl-3.0.html.en>](https://www.gnu.org/licenses/lgpl-3.0.html.en))

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

-   SocketServer respective SocketClient instance
-   SocketContextFactory
-   SocketContext

Let\'s have a look at how these three components are related to each other by implementing a simple networking application.

## An "Echo" Application

Imagine we want to create a very basic TCP (**stream**)/IPv4 (**in**) server/client pair which sends some plain text data unencrypted (**legacy**) to each other in a ping-pong way.

The client shall start sending text data to the server and the server shall reflect that data back to the client. The client receives this reflected data and sends it back again to the server. This data ping-pong shall last infinitely long.

The code of this demo application can be found on [github](https://github.com/VolkerChristian/echo).

### SocketServer and SocketClient Instances

For the server role we just need to create an object of type

``` c++
net::in::stream::legacy::SocketServer<SocketContextFactory>
```

called a *server instance* and for the client role an object of type

``` c++
net::in::stream::legacy::SocketClient<SocketContextFactory>
```

called *client instance* is needed.

A class `SocketContextFactory` is used for both instances as template argument. Such a `SocketContextFactory` is used internally by the `SocketServer` and the `SocketClient` instances to create a concrete `SocketContext` object for each established connection. This `SocketContext` represents a concrete application protocol.

Both instance-classes have, among others, a *default constructor* and a *constructor expecting an instance name* as argument. 

- When the default constructor is used to create the instance object this instance is called an *anonymous instance*.
- In contrast to a *named instance* if the constructors expecting a `std::string` is used for instance creation. 
- For named instances *command line arguments and configuration file entries are automatically created* to configure the instance.

Therefore, for our echo application, we need to implement the application logic (application protocol) for server and client in classes derived from `core::socket::stream::SocketContext`, the base class of all connection-oriented (stream) application protocols and factories derived from `core::socket::stream::SocketContextFactory`.

### SocketContextFactories

Let\'s focus on the SocketContextFactories for our server and client first.

All what needs to be done is to implement a pure virtual method `create()` witch expects a pointer to a `core::socket::stream::SocketConnection` as argument and returns a concrete application `SocketContext`.

The `core::socket::stream::SocketConnection` object involved is managed internally by SNode.C and represents the *physical connection* between the server and a client. Such a `core::socket::stream::SocketConnection` is needed by the `core::socket::stream::SocketContext` to *handle the physical data transfer* between server and client.

#### Echo-Server `SocketContextFactory`

The `create()` method of our `EchoServerContextFactory` returns the `EchoServerContext` whose implementation is presented in the [SocketContexts](#SocketContexts) section below.

Note, that the pointer to the  `core::socket::stream::SocketConnection` is passed as argument to the constructor of our `EchoServerContext`.

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

#### Echo-Client `SocketContextFactory`

The `create()` method of our `EchoClientContextFactory` returns the `EchoClientContext` whose implementation is also presented in the [SocketContexts](#SocketContexts) section below.

Note, that the pointer to the  `core::socket::stream::SocketConnection` is passed as argument to the constructor of our `EchoServerContext`.

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

It is also not difficult to implement the `SocketContext` classes for the server and the client.

-   Remember, the required functionality: The server shall reflect the received data back to the client!
-   And also remember we need to derive from the base class `core::socket::stream::SocketContext`.
-   And at last remember that the class  `core::socket::stream::SocketContext` needs the `core::socket::stream::SocketConnection`  to handle the physical data exchange. Thus, we have to pass the pointer to the `core::socket::stream::SocketConnection` to the constructor of the base class `core::socket::stream::SocketContext`.

The base class `core::socket::stream::SocketContext` provides *some virtual methods* which can be overridden in an concrete `SocketContext` class. These methods will be *called by the framework automatically*.

#### Echo-Server `SocketContext`

For our echo server application it would be sufficient to override the `onReceivedFromPeer()` method only. This method is called by the framework in case some data have already been received from the client. Nevertheless, for more information of what is going on in behind the methods `onConnected` and `onDisconnected` are overridden also.

In the `onReceivedFromPeer()` method we can retrieve data that has already been received by SNode.C using the `readFromPeer()` method provided by the `core::socket::stream::SocketContext` class.

Sending data to the client is done using the method `sendToPeer()`, which is also provided by the `core::socket::stream::SocketContext` class.

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

#### Echo-Client `SocketContext`

The echo client `SocketContext`, unlike the server `SocketContext`, *needs* an overridden `onConnected` method to *initiate* the ping-pong data exchange.

Like in the `EchoServerContext`, `readFromPeer()` and `sendToPeer()` is used in the `onReceivedFromPeer()` method. In addition `sendToPeer()` is also used in the `onConnected()` method to initiate the ping-pong data exchange.

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

Now we can put everything together and implement the main server and client applications. Here *anonymous instances* are used. Thus, we will not get command line arguments automatically.

Note the use of our previously implemented `EchoServerContextFactory` and `EchoClientContextFactory` as template arguments.

At the very beginning SNode.C must be *initialized* by calling

- `core::SNodeC::init(argc, argv)`

and at the end of the main applications the *event-loop* of SNode.C is started by calling

- `core::SNodeC::start()`.

#### Echo-Server Main Application

The server instance `echoServer` must be *activated* by calling `echoServer.listen()`.

SNode.C provides a view overloaded `listen()` methods whose arguments vary depending on the network layer (IPv4, IPv6, RFCOMM, L2CAP, or unix domain sockets) used. Though, every `listen()` method expects a *lambda function* as last argument. Here we use IPv4 and the `listen()` method which expects a port number as argument.

If we would have created a named server instance than a special `listen()` method which only expects the *lambda function* as argument can be used. In that case the configuration of this named instance would be done using command line arguments and/or a configuration file.

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
            std::cout << "Error: Echo server listening on " << socketAddress.toString() 
                      << ": " << strerror(err) << std::endl;
        }
    });

    return core::SNodeC::start(); // Start the event loop, daemonize if requested.
}
```

#### Echo-Client Main Application

The client instance `echoClient` must be *activated* by calling `echoClient.connect()`.

Equivalent to the server instance a client instance provides a view overloaded `connect()` methods whose arguments also vary depending on the network layer used. Here it is assumed that we talk to an IPv4 server which runs on the same machine (`localhost`) as the client. Thus we pass the host name `localhost` and port number `8001` to the `connect()` method.

If we would have created a named client instance than a special `connect()` method which only expects the *lambda function* can be used. In that case the configuration of this named instance would be done using command line arguments and/or a configuration file.

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
                               std::cout << "Error: Echo client connected to " << socketAddress.toString() 
                                         << ": " << strerror(err) << std::endl;
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

  - [`core::socket::stream::SocketContext`](https://volkerchristian.github.io/snode.c-doc/html/classcore_1_1socket_1_1stream_1_1_socket_context.html)

- The framework provides

  - ready to use server and client template classes for each network/transport layer combination.

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

Currently only connection-oriented protocols (SOCK_STREAM) for all supported network layer and transport layer combinations are implemented (for IPv4 and IPv6 this means TCP).

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

As said above in the transport layer section, SSL/TLS encryption is provided transparently for all of these application layer protocols.

# Existing Server- and Client-Classes

Before focusing explicitly on the server and client classes a few common aspects for all network/transport-layer combinations needs to be known.

## `SocketAddress`

Every network layer provides its specific `SocketAddress` class. In typical scenarios you need not bother about these classes as they are managed internally by the framework.

Nevertheless, for the sake of completeness, all currently supported `SocketAddress` classes along with the header files they are declared in are listed below.

Every `SocketServer` and `SocketClient` class has it's specific `SocketAddress` type attached as nested data type. Thus, one can always get the correct `SocketAddress` type buy just

```cpp
using SocketAddress = <ConcreteServerOrClientType>::SocketAddress;
```

as can be seen in the Echo-Demo-Application above.

| Network Layer       | `SocketAddress` Classes                                      | SocketAddress Header Files                                   |
| ------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| IPv4                | [`net::in::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1in_1_1_socket_address.html) | [`net/in/SocketAddress.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2in_2_socket_address_8h.html) |
| IPv6                | [`net::in6::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1in6_1_1_socket_address.html) | [`net/in6/SocketAddress.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2in6_2_socket_address_8h.html) |
| Unix Domain Sockets | [`net::un::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1un_1_1_socket_address.html) | [`net/un/SocketAddress.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2un_2_socket_address_8h.html) |
| Bluetooth RFCOMM    | [`net::rc::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1rc_1_1_socket_address.html) | [`net/rc/SocketAddress.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2rc_2_socket_address_8h.html) |
| Bluetooth L2CAP     | [`net::l2::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1l2_1_1_socket_address.html) | [`net/l2/SocketAddress.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2l2_2_socket_address_8h.html) |

Each `SocketAddress` class provides it's very specific set of constructors.

### `SocketAddress` Constructors

The default constructors of all `SocketAddress` classes creates wild-card `SocketAddress` objects. For a `SocketClient` for exampe, which uses such a wild-card `SocketAddress` as *local address* the operating system chooses a valid `sockaddr` structure automatically.

| SocketAddress                                                | Constructors                                                 |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| [`net::in::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1in_1_1_socket_address.html) | `SocketAddress()`<br/>`SocketAddress(uint16_t port)`<br/>`SocketAddress(const std::string& ipOrHostname)`<br/>`SocketAddress(const std::string& ipOrHostname, uint16_t port)` |
| [`net::in6::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1in6_1_1_socket_address.html) | `SocketAddress()`<br/>`SocketAddress(uint16_t port)`<br/>`SocketAddress(const std::string& ipOrHostname)`<br/>`SocketAddress(const std::string& ipOrHostname, uint16_t port)` |
| [`net::un::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1un_1_1_socket_address.html) | `SocketAddress()`<br/>`SocketAddress(const std::string& sunPath)` |
| [`net::rc::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1rc_1_1_socket_address.html) | `SocketAddress()`<br/>`SocketAddress(uint8_t channel)`<br/>`SocketAddress(const std::string& btAddress)`<br/>`SocketAddress(const std::string& btAddress, uint8_t channel)` |
| [`net::l2::SocketAddress`](https://volkerchristian.github.io/snode.c-doc/html/classnet_1_1l2_1_1_socket_address.html) | `SocketAddress()`<br/>`SocketAddress(uint16_t psm)`<br/>`SocketAddress(const std::string& btAddress)`<br/>`SocketAddress(const std::string& btAddress, uint16_t psm)` |

## `SocketConnection`

Every network layer also provides its specific `SocketConnection` class. Such a `SocketConnection` object represents the *physical connection* to the peer. Equivalent to the `SocketAddress` type, each `SocketServer` and `SocketClient` class provides its correct `SocketConnection` as nested data type. This type can be obtained from a concrete `SocketServer` or `SocketClient` class using

```c++
using SocketConnection = <ConcreteServerOrClientType>::SocketConnection;
```

Each `SocketConnection` object provides, among others, the method

- `int socketConnection->getFd()`

which returns the underlying descriptor used for communication.

Additionally the `SocketConnection` objects of a SSL/TLS `SocketServer` or `SocketClient` class provides the method

- `SSL* socketConnection->getSSL()`

which returns a pointer to the `SSL` structure of *OpenSSL* used for encryption, authenticating and authorization. Using this `SSL` structure one can modify the SSL/TLS behavior before SSL/TLS handshake in the `onConnect()` callback, discussed below, and add all kinds of authentication and authorization logic directly in the `onConnected()` callback, also discussed below.

| Encryption | SocketConnection Classes                                     | SocketConnection Header Files                                |
| ---------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| Legacy     | [`core::socket::stream::legacy::SocketConnection`](https://volkerchristian.github.io/snode.c-doc/html/classcore_1_1socket_1_1stream_1_1legacy_1_1_socket_connection.html) | [`core/socket/stream/legacy/SocketConnection.h`](https://volkerchristian.github.io/snode.c-doc/html/stream_2legacy_2_socket_connection_8h.html) |
| SSL/TLS    | [`core::socket::stream::tls::SocketConnection`](https://volkerchristian.github.io/snode.c-doc/html/classcore_1_1socket_1_1stream_1_1tls_1_1_socket_connection.html) | [`core/socket/stream/tls/SocketConnection.h`](https://volkerchristian.github.io/snode.c-doc/html/stream_2tls_2_socket_connection_8h.html) |

### Most Important common `SocketConnection` Methods

| Method                                                       | Explanation                                                  |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| `int getFd()`                                                | Returns the underlying descriptor used for communication.    |
| `SSL* getSSL()`                                              | Returns the pointer to the OpenSSL SSL structure.            |
| `void sendToPeer(const char* junk, std::size_t junkLen)` <br/>`void sendToPeer(const std::string& data)`<br />`void sendToPeer(const std::vector<int8_t>& data)`<br />`void sendToPeer(const std::vector<char>& data)` | Enqueue data to be sent to peer.                             |
| `std::size_t readFromPeer(char* junk, std::size_t junkLen)`  | Read already received data from peer.                        |
| `void shutdownRead()`<br/>`void shuddownWrite()`             | Shut down socket either for reading or writing.              |
| `void close()`                                               | Hard close the connection without a prior shutdown.          |
| `void setTimeout(utils::Timeval& timeout)`                   | Set the inactivity timeout of a connection (default 60 seconds).<br/>If no data has been transfered within this amount of time<br/>the connection is terminated. |
| `bool isValid()`                                             | Check if a connection has been created successfully.         |

## Constructors of `SocketServer` and `SocketClient` Classes

Beside the already discussed constructors of the `SocketServer` and `SocketClient` classes, each of them provides two additional constructors which expect three callback `std::function`s as arguments. This callback functions are called by SNode.C during connection establishment and connection shutdown between server and clients.

Thus the full list of constructors of the `SocketServer` and `SocketClient` classes is:

### Constructors of `SocketServer` Classes

```c++
SocketServer()
    
SocketServer(const std::function<void(SocketServer::SocketConnection*)>& onConnect,
             const std::function<void(SocketServer::SocketConnection*)>& onConnected,
             const std::function<void(SocketServer::SocketConnection*)>& onDisconnect)
    
SocketServer(const std::string& name)
    
SocketServer(const std::string& name,
             const std::function<void(SocketServer::SocketConnection*)>& onConnect,
             const std::function<void(SocketServer::SocketConnection*)>& onConnected,
             const std::function<void(SocketServer::SocketConnection*)>& onDisconnect)
```

### Constructors of `SocketClient` Classes

```c++
SocketClient()
    
SocketClient(const std::function<void(SocketClient::SocketConnection*)>& onConnect,
             const std::function<void(SocketClient::SocketConnection*)>& onConnected,
             const std::function<void(SocketClient::SocketConnection*)>& onDisconnect)
    
SocketClient(const std::string& name)
    
SocketClient(const std::string& name,
             const std::function<void(SocketClient::SocketConnection*)>& onConnect,
             const std::function<void(SocketClient::SocketConnection*)>& onConnected,
             const std::function<void(SocketClient::SocketConnection*)>& onDisconnect)
```

### Constructor Callbacks

All three callbacks expect a pointer to a `SocketConnection` as argument. This `SocketConnection` can be used to modify all aspects of the physical connection.

***Important***: Do not confuse this callbacks with the overridden `onConnected()` and `onDisconnected()` methods of a `SocketConetext` as this virtual methods are called in case a `SocketContext` object has been created successfully or before being destroyed.

#### The `onConnect()` Callback

This callback is called after a SOCK_STREAM connection has been created successful.

For a `SocketServer` this means after an internal successful call to

- `accept()`

and for a `SocketServer` after an internal successful call to

- `connect()`

This does not necessarily mean that the connection is ready for communication. Especially in case of an SSL/TLS connection the initial SSL/TLS handshake has yet not been done.

#### The `onConnected()` Callback

This callback is called after the connection has fully established and is ready for communication. In case of an SSL/TLS connection the initial SSL/TLS handshake has been successfully finished.

In case of an legacy connection `onConnected()` is called immediately after `onConnect()` because no additional handshake needs to be done.

#### The `onDisconnected()` Callback

As the name suggests this callback is executed after a connection to the peer has been shut down.

### Attaching the Callbacks during Instance Creation

For a concrete `SocketServer` instance (here an anonymous instance) the constructors expecting callbacks are used like

```c++
using EchoServer = net::in::stream::legacy::SocketServer<EchoServerContextFactory>;
using SocketAddress = EchoServer::SocketAddress;
using SocketConnection = EchoServer::SocketConnection;

EchoServer echoServer([] (SocketConnection* socketConnection) -> void {
                          std::cout << "Connection to peer estableshed" << std::endl;
                      },
                      [] (SocketConnection* socketConnection) -> void {
                          std::cout << "Connection to peer ready to be used" << std::endl;
                      },
                      [] (SocketConnection* socketConnection) -> void {
                          std::cout << "Connection to peer closed" << std::endl;
                      });

echoServer.listen(...);
```

and for a concrete `SocketClient` class like

```c++
using EchoClient = net::in::stream::legacy::SocketServer<EchoClientContextFactory>;
using SocketAddress = EchoClient::SocketAddress;
using SocketConnection = EchoClient::SocketConnection;

EchoClient echoClient([] (SocketConnection* socketConnection) -> void {
                          std::cout << "Connection to peer estableshed" << std::endl;
                      },
                      [] (SocketConnection* socketConnection) -> void {
                          std::cout << "Connection to peer ready to be used" << std::endl;
                      },
                      [] (SocketConnection* socketConnection) -> void {
                          std::cout << "Connection to peer closed" << std::endl;
                      });

echoClient.connect(...);
```

### Attaching the Callbacks to `SocketServer` and `SocketClient` Instances

In case `SocketServer` and `SocketClient` instances have been created using the constructors not expecting those three callbacks they can be attached to this instances afterwards by using the methods

- `void setOnConnect(const std::function<SocketConnection*>& onConnect)`
- `void setOnConnected(const std::function<SocketConnection*>& onConnected)`
- `void setOnDisconnected(const std::function<SocketConnection*>& onDisconnected)`

like for example

```c++
using EchoServer = net::in::stream::legacy::SocketServer<EchoServerContextFactory>;
using SocketAddress = EchoServer::SocketAddress;
using SocketConnection = EchoServer::SocketConnection;

EchoServer echoServer;

echoServer.setOnConnect([] (SocketConnection* socketConnection) -> void {
    std::cout << "Connection to peer estableshed" << std::endl;
});

echoServer.setOnConnected([] (SocketConnection* socketConnection) -> void {
    std::cout << "Connection to peer ready to be used" << std::endl;
});
    
echoServer.setOnDisconnected([] (SocketConnection* socketConnection) -> void {
    std::cout << "Connection to peer closed" << std::endl;
});

echoServer.listen(...);
```

and

```c++
using EchoClient = net::in::stream::legacy::SocketServer<EchoClientContextFactory>;
using SocketAddress = EchoClient::SocketAddress;
using SocketConnection = EchoClient::SocketConnection;

EchoClient echoClient;

echoClient.setOnConnect([] (SocketConnection* socketConnection) -> void {
    std::cout << "Connection to peer estableshed" << std::endl;
});

echoClient.setOnConnected([] (SocketConnection* socketConnection) -> void {
    std::cout << "Connection to peer ready to be used" << std::endl;
});
    
echoClient.setOnDisconnected([] (SocketConnection* socketConnection) -> void {
    std::cout << "Connection to peer closed" << std::endl;
});

echoClient.connect(...);
```

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


| Network Layer       | Legacy Connection                                            | SSL/TLS Connection                                           |
| ------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| IPv4                | [`net/in/stream/legacy/SocketServer.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2in_2stream_2legacy_2_socket_server_8h.html) | [`net/in/stream/tls/SocketServer.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2in_2stream_2tls_2_socket_server_8h.html) |
| IPv6                | [`net/in6/stream/legacy/SocketServer.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2in6_2stream_2legacy_2_socket_server_8h.html) | [`net/in6/stream/tls/SocketServer.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2in6_2stream_2tls_2_socket_server_8h.html) |
| Unix Domain Sockets | [`net/un/stream/legacy/SocketServer.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2un_2stream_2legacy_2_socket_server_8h.html) | [`net/un/stream/tls/SocketServer.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2un_2stream_2tls_2_socket_server_8h.html) |
| Bluetooth RFCOMM    | [`net/rc/stream/legacy/SocketServer.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2rc_2stream_2legacy_2_socket_server_8h.html) | [`net/rc/stream/tls/SocketServer.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2rc_2stream_2tls_2_socket_server_8h.html) |
| Bluetooth L2CAP     | [`net/l2/stream/legacy/SocketServer.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2l2_2stream_2legacy_2_socket_server_8h.html) | [`net/l2/stream/tls/SocketServer.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2l2_2stream_2tls_2_socket_server_8h.html) |

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

| IPv4 `listen()` Methods                                      |
| ------------------------------------------------------------ |
| `void listen(uint16_t port, StatusFunction& onError)`        |
| `void listen(uint16_t port, int backlog, StatusFunction& onError)` |
| `void listen(const std::string& ipOrHostname, uint16_t port, StatusFunction& onError)` |
| `void listen(const std::string& ipOrHostname, uint16_t port, int backlog, StatusFunction& onError)` |

##### IPv6 specific `listen()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::in6::SocketAddress&, int)>;
```

For the IPv6/SOCK_STREAM combination exist four specific `listen()` methods.

| IPv6 `listen()` Methods                                      |
| ------------------------------------------------------------ |
| `void listen(uint16_t port, StatusFunction& onError)`        |
| `void listen(uint16_t port, int backlog, StatusFunction& onError)` |
| `void listen(const std::string& ipOrHostname, uint16_t port, StatusFunction& onError)` |
| `void listen(const std::string& ipOrHostname, uint16_t port, int backlog, StatusFunction& onError)` |

##### Unix Domain Socket specific `listen()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::un::SocketAddress&, int)>;
```

For the Unix Domain Socket/SOCK_STREAM combination exist two specific `listen()` methods.

| Unix-Domain `listen()` Methods                               |
| ------------------------------------------------------------ |
| `void listen(const std::string& sunPath, StatusFunction& onError)` |
| `void listen(const std::string& sunPath, int backlog, StatusFunction& onError)` |

##### Bluetooth RFCOMM specific `listen()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::rc::SocketAddress&, int)>;
```

For the RFCOMM/SOCK_STREAM combination exist four specific `listen()` methods.

| Bluetooth RFCOMM `listen()` Methods                          |
| ------------------------------------------------------------ |
| `void listen(uint8_t channel, StatusFunction& onError)`      |
| `void listen(uint8_t channel, int backlog, StatusFunction& onError)` |
| `void listen(const std::string& btAddress, uint8_t channel, StatusFunction& onError)` |
| `void listen(const std::string& btAddress, uint8_t channel, int backlog, StatusFunction& onError)` |

##### Bluetooth L2CAP specific `listen()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::l2::SocketAddress&, int)>;
```

For the L2CAP/SOCK_STREAM combination exist four specific `listen()` methods.

| Bluetooth L2CAP `listen()` Methods                           |
| ------------------------------------------------------------ |
| `void listen(uint16_t psm, StatusFunction& onError)`         |
| `void listen(uint16_t psm, int backlog, StatusFunction& onError)` |
| `void listen(const std::string& btAddress, uint16_t psm, StatusFunction& onError)` |
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


| Network Layer       | Legacy Connection                                            | SSL/TLS Connection                                           |
| ------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
| IPv4                | [`net/in/stream/legacy/SocketClient.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2in_2stream_2legacy_2_socket_client_8h.html) | [`net/in/stream/tls/SocketClient.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2in_2stream_2tls_2_socket_client_8h.html) |
| IPv6                | [`net/in6/stream/legacy/SocketClient.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2in6_2stream_2legacy_2_socket_client_8h.html) | [`net/in6/stream/tls/SocketClient.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2in6_2stream_2tls_2_socket_client_8h.html) |
| Unix Domain Sockets | [`net/un/stream/legacy/SocketClient.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2un_2stream_2legacy_2_socket_client_8h.html) | [`net/un/stream/tls/SocketClient.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2un_2stream_2tls_2_socket_client_8h.html) |
| Bluetooth RFCOMM    | [`net/rc/stream/legacy/SocketClient.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2rc_2stream_2legacy_2_socket_client_8h.html) | [`net/rc/stream/tls/SocketClient.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2rc_2stream_2tls_2_socket_client_8h.html) |
| Bluetooth L2CAP     | [`net/l2/stream/legacy/SocketClient.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2l2_2stream_2legacy_2_socket_client_8h.html) | [`net/l2/stream/tls/SocketClient.h`](https://volkerchristian.github.io/snode.c-doc/html/net_2l2_2stream_2tls_2_socket_client_8h.html) |

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

#### Specific `connect()` Methods

##### IPv4 specific `connect()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::in::SocketAddress&, int)>;
```

For the IPv4/SOCK_STREAM combination exist four specific `connect()` methods.

| IPv4 `connect()` Methods                                     |
| ------------------------------------------------------------ |
| `void connect(const std::string& ipOrHostname, uint16_t port, StatusFunction& onError)` |
| `void connect(const std::string& ipOrHostname, uint16_t port, uint16_t bindPort, StatusFunction& onError)` |
| `void connect(const std::string& ipOrHostname, uint16_t port, const std::string& bindIpOrHostname, StatusFunction& onError)` |
| `void connect(const std::string& ipOrHostname, uint16_t port, const std::string& bindIpOrHostname, uint16_t bindPort, StatusFunction& onError)` |

##### IPv6 specific `connect()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::in6::SocketAddress&, int)>;
```

For the IPv6/SOCK_STREAM combination exist four specific `connect()` methods.

| IPv6 `connect()` Methods                                     |
| ------------------------------------------------------------ |
| `void connect(const std::string& ipOrHostname, uint16_t port, StatusFunction& onError)` |
| `void connect(const std::string& ipOrHostname, uint16_t port, uint16_t bindPort, StatusFunction& onError)` |
| `void connect(const std::string& ipOrHostname, uint16_t port, const std::string& bindIpOrHostname, StatusFunction& onError)` |
| `void connect(const std::string& ipOrHostname, uint16_t port, const std::string& bindIpOrHostname, uint16_t bindPort, StatusFunction& onError)` |

##### Unix Domain Socket specific `connect()` Methods

The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::un::SocketAddress&, int)>;
```

For the Unix Domain Socket/SOCK_STREAM combination exist two specific `connect()` methods.

| Unix-Domain `connect()` Methods                              |
| ------------------------------------------------------------ |
| `void connect(const std::string& sunPath, StatusFunction& onError)` |
| `void connect(const std::string& remoteSunPath, const std::string& localSunPath, StatusFunction& onError)` |

##### Bluetooth RFCOMM specific `connect()` Methods

IPv4 The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::rc::SocketAddress&, int)>;
```

For the RFCOMM/SOCK_STREAM combination exist four specific `connect()` methods.

| Bluetooth RFCOMM `connect()` Methods                         |
| ------------------------------------------------------------ |
| `void connect(const std::string& btAddress, uint8_t channel, StatusFunction& onError)` |
| `void connect(const std::string& btAddress, uint8_t channel, uint8_t bindChannel, StatusFunction& onError)` |
| `void connect(const std::string& btAddress, uint8_t channel, const std::string& bindBtAddress,StatusFunction& onError)` |
| `void connect(const std::string& btAddress, uint8_t channel, const std::string& bindBtAddress, uint8_t bindChannel, StatusFunction& onError)` |

##### Bluetooth L2CAP specific `connect()` Methods

IPv4 The type `StatusFunction` is defined as

```cpp
using StatusFunction = const std::function<void(const net::l2::SocketAddress&, int)>;
```

For the L2CAP/SOCK_STREAM combination exist four specific `connect()` methods.

| Bluetooth L2CAP `connect()` Methods                          |
| ------------------------------------------------------------ |
| `void connect(const std::string& btAddress, uint16_t psm, StatusFunction& onError)` |
| `void connect(const std::string& btAddress, uint16_t psm, uint16_t bindPsm, StatusFunction& onError)` |
| `void connect(const std::string& btAddress, uint16_t psm, const std::string& bindBtAddress, StatusFunction& onError)` |
| `void connect(const std::string& btAddress, uint16_t psm, const std::string& bindBtAddress, uint16_t bindPsm, StatusFunction& onError)` |

# Configuration

Each `SocketServer` and `SocketClient` instance needs to be configured before they can be started by the SNode.C event loop. Fore instance, a IPv4/TCP `SocketServer` needs to know at least the port number it should listen on, a IPv4/TCP `SocketClient` needs to now the host name or the IPv4 address and the port number a server is listening on. And if an SSL/TLS instance is used certificates are necessary for successful encryption.

There are many more configuration items but lets focus on those mentioned above.

## Three different Options for Configuration

SNode.C provides three different options to specify such configuration items. Nevertheless, internally all uses the same underlying configuration system, which is entirely based on the great [CLI11: Command line parser for C++11](https://github.com/CLIUtils/CLI11) library.

The configuration can either be done via

- the provided C++ API directly in the source code for anonymous and named instances
- command line arguments for named instances
- configuration files for named instances

Command line and configuration file configuration is available and valid for server and client instances created before calling `core::SnodeC::start()`. For subsequently created instances, configuration can only be done with the C++ API.

## Configuration using the C++ API

Each anonymous and named `SocketServer` and `SocketClient` instance provide an configuration object which could be obtained by calling the method `getConfig()` on the instance, which returns a reference to that configuration object.

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
        std::cout << "Error: Echo server listening on " << socketAddress.toString() 
                  << ": " << perror("") << std::endl;
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
        std::cout << "Error: Echo client connected to " << socketAddress.toString() 
                  << ": " << perror("") << std::endl;
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

- Persistent options can be stored in the configuration file by appending `-w` on the command line.
- Non-persistent options are never stored in the configuration file.

#### Instance Configuration

Each named `SocketServer` and `SocketClient` instance get their specific set of command line options accessible by specifying the name of the instance on the command line.

Thus, for instance if the `EchoServer` instance is created using and instance name as argument to the server instance constructor like for example 

```cpp
EchoServer echoServer("echo"); // Create named server instance
```

 (try it yourself using the code from github in the **[named-instance](https://github.com/VolkerChristian/echo/tree/named-instance)** branch of the echo application), the output of the help screen changes slightly:

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

Note that now the named instance *echo* appears at the end of the help screen.

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
```

on screen.

#### Sections

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
Success: Echo server listening on 0.0.0.0:8080
```

the in the configuration file stored port number `8080` is used instead of the port number `8001` used directly in the code.

All existing configuration options specified directly in the application code can be overridden on the command line and/or the configuration file in that way.

### Anatomy of the Command Line Interface

The command line interface follows a well defined structure, for example

```shell
command@line:~/> serverorclientexecutable --app-opt1 val1 --app-opt2 val2 instance1 --inst-opt1 val11 --int-opt2 val12 section11 --sec1-opt1 val111 --sec1-opt2 val112 section12 --sec2-opt1 val121 --sec2-opt2 val122 instance2 --inst-opt1 val21 --inst-opt2 val22 section21 --sec1-opt1 val211 --sec1-opt2 val212 section22 --sec2-opt1 val221 --sec2-opt2 val222
```

if two instances with instance names *instance1* and *instance2* are present in the executable.

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
        std::cout << "Error: Echo server listening on " << socketAddress.toString() 
                  << ": " << strerror(err) << std::endl;
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
Success: Echo server listening on 0.0.0.0:8080
```

Again, this configuration can be made permanent by writing the configuration file by appending `-w` to the command line:

```shell
command@line:~/> echoserver echo local --port 8080 -w
Writing config file: /home/[user]/.config/snode.c/echoserver.conf

command@line:~/> echoserver
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

## Configuration via a Configuration File

Each configuration option which is marked as *persistent* in the help output can also be configured via a configuration file. The names of the entries for options in such an configuration file follow a well defined structure.

```ini
instancename.sectionname.optionname = value
```

Thus to configure the port number the *echoserver* with server instance name `echo`shall listen on the entry in the configuration file would look like

```ini
echo.local.port = 8080
```

and for the *echoclient* the remote hostname or ip-address and the remote port number is configured by specifying

```ini
echo.remote.host = "localhost"
echo.remote.port = 8080
```

The content of an configuration file can be printed on screen by appending the flag `-s` on the command line.

Thus, the configuration of the *echoserver* configured and made persistent on the command line above to listen on port number 8080 can be shown using

```shell
command@line:~/> echoserver -s
```

which leads to the output

```ini
## Configuration for Application 'echoserver'

## Options (persistent)
# Log level
#log-level=3

# Verbose level
#verbose-level=0

# Logfile path
#log-file="/home/[user]/.local/log/snode.c/echoserver.log"

# Enforce writing of logs to file for foreground applications
#enforce-log-file=false

# Start application as daemon
#daemonize=false

# Run daemon under specific user permissions
#user-name="[user]"

# Run daemon under specific group permissions
#group-name="[user]"


## Configuration for server instance 'echo'

## Options (persistent)
# Disable this instance
#echo.disable=false


## Local side of connection for instance 'echo'

## Options (persistent)
# Host name or IPv4 address
#echo.local.host="0.0.0.0"

# Port number
#echo.local.port=8001
echo.local.port=8080


## Configuration of established connections for instance 'echo'

## Options (persistent)
# Read timeout in seconds
#echo.connection.read-timeout=60

# Write timeout in seconds
#echo.connection.write-timeout=60

# Read block size
#echo.connection.read-block-size=16384

# Write block size
#echo.connection.write-block-size=16384

# Terminate timeout
#echo.connection.terminate-timeout=1


## Configuration of socket behaviour for instance 'echo'

## Options (persistent)
# Reuse socket address
#echo.socket.reuse-address=false

# Reuse port number
#echo.socket.reuse-port=false


## Configuration of server socket for instance 'echo'

## Options (persistent)
# Listen backlog
#echo.server.backlog=5

# Accepts per tick
#echo.server.accepts-per-tick=1
```

***Note***: Options with default values, which includes option values configured in-code, are commented in the configuration file.

### Default Name of a Configuration File

The default name of a configuration file is simple the application name to which `.conf` is appended.

Thus, for the *echoserver* and the *echoclient* the names of their configuration files are

- `echoserver.conf`
- `echoclient.conf`

### Location of Configuration Files

Configuration files are stored in directories following the [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html). So these directories differ if an application is run as ordinary user or the root user.

- **Ordinary User**: `$HOME/.config/snode.c/`
- **Root User**: `/etc/snode.c/`

This directories are created automatically by an application in case they did not exist.

## Important Configuration Sections

### SSL/TLS Configuration (Section *tls*)

SSL/TLS encryption is provided if a SSL/TLS server or client instance is created. Internally OpenSSL is used.

Equivalent to all other configuration options SSL/TLS encryption can be configured either in-code, on the command line or via configuration files.

In most scenarios it is sufficient to specify a CA-certificate, a certificate chain, a certificate key and, in case the key is encrypted, a password.

***Important**:* CA-certificate, certificate chain, and certificate key needs to be in *PEM format*.

In case a CA-certificate is configured either on the server and/or the client side a certificate request is send to the peer during the initial SSL/TLS handshake. In case the peer answers with an certificate response this response can be validated in-code in the `onConnected` callback of a `SocketServer` or `SocketClient` class.

***Warning***: Passwords either provided in-code or on the command line are not obfuscated in memory during runtime. Thus it is a good idea to not specify a password in-code or on the command line. In that case OpenSSL asks for the password during application startup. This password is only used internally by OpenSSL and is removed from memory as soon as the keys have been decrypted.

***Warning***: Hold all keys and their passwords secured!

#### SSL/TLS In-Code Configuration

If the echo server instance would have been created using an SSL/TLS-SocketServer class like e.g.

```c++
using EchoServer = net::in::stream::tls::SocketServer<EchoServerContextFactory>;
using SocketAddress = EchoServer::SocketAddress;

EchoServer echoServer("echo");
```

than SSL/TLS could be configured in-code using

```c++
echoServer.getConfig().setCaCertFile("<path to X.509 CA certificate>");   // Has to be in PEM format
echoServer.getConfig().setCertChain("<path to X.509 certificate chain>"); // Has to be in PEM format
echoServer.getConfig().setCertKey("<path to X.509 certificate key>");     // Has to be in PEM format
echoServer.getConfig().setCertKeyPassword("<certificate key password>");
```

The same technique can be used for the client instance

```c++
using EchoClient = net::in::stream::tls::SocketClient<EchoClientContextFactory>;
using SocketAddress = EchoClient::SocketAddress;

EchoClient echoClient("echo");

echoClient.getConfig().setCaCertFile("<path to X.509 CA certificate>");   // Has to be in PEM format
echoClient.getConfig().setCertChain("<path to X.509 certificate chain>"); // Has to be in PEM format
echoClient.getConfig().setCertKey("<path to X.509 certificate key>");     // Has to be in PEM format
echoClient.getConfig().setCertKeyPassword("<certificate key password>");
```

#### SSL/TLS Command Line Configuration

*Note*, that the in-code configuration can be overridden on the command line like all other configuration items.

Using this SSL/TLS echo-instance the help screen for this *echo* instance offers an additional section *tls* which provides the command line arguments to configure SSL/TLS behaviour.

```shell
command@line:~/> echoserver echo --help
Configuration for server instance 'echo'

Usage: echoserver echo [OPTIONS] [SECTIONS]

Options (none persistent):
  -h,--help
       Print this help message and exit
  -a,--help-all
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
  tls  Configuration of SSL/TLS behaviour for instance 'echo'
```

```shell
command@line:~/> echoserver echo tls --help
Configuration of SSL/TLS behaviour for instance 'echo'

Usage: echoserver echo tls [OPTIONS]

Options (none persistent):
  -h,--help
       Print this help message and exit
  -a,--help-all
       Print this help message and exit

Options (persistent):
  --cert-chain filename:PEM-FILE [<path to X.509 certificate chain>] 
       Certificate chain file
  --cert-key filename:PEM-FILE [<path to X.509 certificate key>] 
       Certificate key file
  --cert-key-password password:TEXT [<certificate key password>] 
       Password for the certificate key file
  --ca-cert-file filename:PEM-FILE [<path to X.509 CA certificate>]
       CA-certificate file
  --ca-cert-dir directory:PEM-CONTAINER 
       CA-certificate directory
  --ca-use-default-cert-dir={true,false} [false] 
       Use default CA-certificate directory
  --cipher-list cipher_list:CHIPHER 
       Cipher list (openssl syntax)
  --tls-options options:UINT [0] 
       OR combined SSL/TLS options (openssl values)
  --init-timeout timeout:POSITIVE [10] 
       SSL/TLS initialization timeout in seconds
  --shutdown-timeout timeout:POSITIVE [2] 
       SSL/TLS shutdown timeout in seconds
  --sni-cert sni <key> value {<key> value} ... {%% sni <key> value {<key> value} ...} ["" "" "" ""] ... 
       Server Name Indication (SNI) Certificates:
       sni = SNI of the virtual server
       <key> = {
                 "CertChain" -> value:PEM-FILE,
                 "CertKey" -> value:PEM-FILE,
                 "CertKeyPassword" -> value:TEXT,
                 "CaCertFile" -> value:PEM-FILE,
                 "CaCertDir" -> value:PEM-CONTAINER:DIR,
                 "UseDefaultCaDir" -> value:BOOLEAN [false],
                 "SslOptions" -> value:UINT
               }
  --force-sni={true,false} [false] 
       Force using of the Server Name Indication
```

All available SSL/TLS options are listed in this help screen. 

Thus the certificates can also bee configured running

```shell
command@line:~/> echoserver echo tls --ca-cert-file <path to X.509 CA certificate> --cert-chain <path to X.509 certificate chain> --cert-key <path to X.509 certificate key> --cert-key-password <certificate key password>
```

The demo code for this application can be found on github in the branch [named-instance-tls](https://github.com/VolkerChristian/echo/tree/named-instance-tls). Included in that branch is the directory *certs* where demo self signed CA certificate authorities with corresponding certificates for server and client can be found. This certificates have been created using the tool [XCA](https://hohnstaedt.de/xca/). The database of XCA for this certificates can also be found in that directory. The password of the XCA database and the keys is always 'snode.c'. 

***Warning***: Do not use this certificates for production! But the database of XCA can be used as template for own certificate creation.

This certificates can be applied to the echo server and client applications either in-code or on the command line like for instance

```shell
command@line:~/> echoserver --verbose-level 10 --log-level 6 echo tls --ca-cert-file [absolute-path-to]/SNode.C-Client-CA.crt --cert-chain [absolute-path-to]/Snode.C-Server-Cert.pem --cert-key [absolute-path-to]/Snode.C-Server-Cert.key --cert-key-password snode.c
```

```shell
command@line:~/> echoclient --verbose-level 10 --log-level 6 echo tls --ca-cert-file [absolute-path-to]/SNode.C-Server-CA.crt --cert-chain [absolute-path-to]/Snode.C-Client-Cert.pem --cert-key [absolute-path-to]/Snode.C-Client-Cert.key --cert-key-password snode.c
```

#### Using SSL/TLS with Other Network Layers

All supported network layers (IPv4, IPv6, Unix-Domain Sockets, RFCOMM, and L2CAP) combined with a connection oriented transport layer (SOCK_STREAM) can be secured with exactly the same technique. This is funny because e.g. bluetooth already provides encryption but can be made even more secure using SSL/TLS. But yes, we get this feature automatically due to the internal architecture of the framework.

### Socket Configuration (Section *socket*)

Via this section socket options can be configured.

#### Common *socket* Options for `SocketServer` and `SocketClient` Instances

Some configuration options are common for all SocketServer and SocketClient instances

| Command Line Configuration      | In-Code Configuration                                     | Description                                                  |
| ------------------------------- | --------------------------------------------------------- | ------------------------------------------------------------ |
| `--reuse-address=[true, false]` | `instance.getConfig().setReuseAddress(bool reuse = true)` | This flag turns on address reuse. Thus if a socket is left in a TIME_WAIT state after application shutdown the address and port/channel/psm number tuple can be reused immediately.<br />***Recommendation***: Leave on `false` for production but set to `true` during development. |

#### Specific *socket* Options for IPv4 and IPv6 `SocketServer` 

| Command Line Configuration   | In-Code Configuration                                  | Description                                                  |
| ---------------------------- | ------------------------------------------------------ | ------------------------------------------------------------ |
| `--reuse-port=[true, false]` | `instance.getConfig().setReusePort(bool reuse = true)` | This flag turns on port reuse so multiple server applications can listening on the same address and port number tuple simultaneously.<br />With this option set to `true` one can start an server application multiple times and the operating system (Linux) routes incoming client connection requests randomly to one of the running application instances. So one can achieve a simple load balancing/multitasking application. |

#### Specific *socket* Options for IPv6 `SocketServer` and `SocketClient`

| Command Line Configuration  | In-Code Configuration                                | Description                                                  |
| --------------------------- | ---------------------------------------------------- | ------------------------------------------------------------ |
| `--ipv6-only=[true, false]` | `instance.getConfig().setIPv6Only(bool only = true)` | By default if a IPv6 socket is created on Linux it is a dual-stack socket also using IPv4 addresses. In case this flag is set to `true` a pure IPv6 socket is created. |

# Using more than one Instance in an Application

SNode.C is designed to run multiple Server and Client Instances in one Application.

For instance, if the echo server shall also communicate via e.g. Unix-Domain sockets and Bluetoot-RFCOMM those two server Instances need to be added but the application level protocol need not to be changed. The code can be found on github in the **[multiple-instances](https://github.com/VolkerChristian/echo/tree/multiple-instances)** branch of the echo application.

In that case the Main-Application would look like

```c++
#include "EchoServerContextFactory.h"

#include <core/SNodeC.h>
#include <iostream>
#include <net/in/stream/legacy/SocketServer.h>
#include <net/rc/stream/legacy/SocketServer.h>
#include <net/un/stream/legacy/SocketServer.h>
#include <string.h>
#include <string>

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using EchoServerIn = net::in::stream::legacy::SocketServer<EchoServerContextFactory>;
    using SocketAddressIn = EchoServerIn::SocketAddress;

    EchoServerIn echoServerIn;
    echoServerIn.listen(8001, [](const SocketAddressIn& socketAddress, int err) -> void { // IPv4, port 8001
        if (err == 0) {
            std::cout << "Success: Echo server listening on " << socketAddress.toString() << std::endl;
        } else {
            std::cout << "Error: Echo server listening on " << socketAddress.toString() 
                      << ": " << strerror(err) << std::endl;
        }
    });

    using EchoServerUn = net::un::stream::legacy::SocketServer<EchoServerContextFactory>;
    using SocketAddressUn = EchoServerUn::SocketAddress;

    EchoServerUn echoServerUn;
    echoServerUn.listen(
        "/tmp/echoserver", [](const SocketAddressUn& socketAddress, int err) -> void { // Unix-domain socket /tmp/echoserver
            if (err == 0) {
                std::cout << "Success: Echo server listening on " << socketAddress.toString() << std::endl;
            } else {
                std::cout << "Error: Echo server listening on " 
                          << socketAddress.toString() << ": " << strerror(err) << std::endl;
            }
        });

    using EchoServerRc = net::rc::stream::legacy::SocketServer<EchoServerContextFactory>;
    using SocketAddressRc = EchoServerRc::SocketAddress;

    EchoServerRc echoServerRc;
    echoServerRc.listen(16, [](const SocketAddressRc& socketAddress, int err) -> void { // Bluetooth RFCOMM on channel 16
        if (err == 0) {
            std::cout << "Success: Echo server listening on " << socketAddress.toString() << std::endl;
        } else {
            std::cout << "Error: Echo server listening on " << socketAddress.toString() 
                      << ": " << strerror(err) << std::endl;
        }
    });

    return core::SNodeC::start(); // Start the event loop, daemonize if requested.
}
```

and the client application with an additional Unix-Domain socket instance look like

```c++
#include "EchoClientContextFactory.h"

#include <core/SNodeC.h>
#include <iostream>
#include <net/in/stream/legacy/SocketClient.h>
#include <net/un/stream/legacy/SocketClient.h>
#include <string.h>
#include <string>

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    static std::string ipv4("IPv4 Socket");
    using EchoClientIn = net::in::stream::legacy::SocketClient<EchoClientContextFactory<ipv4>>;
    using SocketAddressIn = EchoClientIn::SocketAddress;

    EchoClientIn echoClientIn; // Create client instance
    echoClientIn.connect("localhost",
                         8001,
                         [](const SocketAddressIn& socketAddress,
                            int err) -> void { // Connect to server
                             if (err == 0) {
                                 std::cout << "Success: Echo connected to " << socketAddress.toString() << std::endl;
                             } else {
                                 std::cout << "Error: Echo client connecting to " << socketAddress.toString() 
                                           << ": " << strerror(err) << std::endl;
                             }
                         });

    static std::string unixDomain("Unix-Domain Socket");
    using EchoClientUn = net::un::stream::legacy::SocketClient<EchoClientContextFactory<unixDomain>>;
    using SocketAddressUn = EchoClientUn::SocketAddress;

    EchoClientUn echoClientUn; // Create client instance
    echoClientUn.connect("/tmp/echoserver",
                         [](const SocketAddressUn& socketAddress,
                            int err) -> void { // Connect to server
                             if (err == 0) {
                                 std::cout << "Success: Echo connected to " << socketAddress.toString() << std::endl;
                             } else {
                                 std::cout << "Error: Echo client connecting to " << socketAddress.toString() 
                                           << ": " << strerror(err) << std::endl;
                             }
                         });

    return core::SNodeC::start(); // Start the event loop, daemonize if requested.
}
```

The `EchoClientContextFactory` and the `EchoClientContext` has also be changed slightly to expect now a `std::string` as non-type template argument which is appended in the ping-pong initiation to the string send to the server to distinguish both instances in the output of the application.

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
    //    ([[maybe_unused]] express::Request& (req), [[maybe_unused]] express::Response& (res))
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
                 "        <pre>" +
                 std::string(reinterpret_cast<char*>(req.body.data())) +
                 "        </pre>"
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

