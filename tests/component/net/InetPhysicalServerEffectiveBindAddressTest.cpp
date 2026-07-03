/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "net/in/SocketAddress.h"
#include "net/in/phy/stream/PhysicalSocketServer.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main() {
    tests::support::TestResult testResult;

    net::in::phy::stream::PhysicalSocketServer physicalServerSocket;
    net::in::SocketAddress configuredAddress("127.0.0.1", 0);

    testResult.expectTrue(physicalServerSocket.open({}, net::in::phy::stream::PhysicalSocketServer::Flags::NONBLOCK) >= 0,
                          "IPv4 physical server socket opens");
    testResult.expectTrue(physicalServerSocket.bind(configuredAddress) == 0,
                          "IPv4 physical server socket binds to loopback port zero");
    testResult.expectTrue(physicalServerSocket.getBindAddress().getPort() != 0,
                          "IPv4 physical server socket reports kernel-selected bind port");

    return testResult.processResult();
}
