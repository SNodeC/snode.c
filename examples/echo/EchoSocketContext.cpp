#include "EchoSocketContext.h"

#include <string>

namespace echo {

    EchoSocketContext::EchoSocketContext(core::socket::stream::SocketConnection* socketConnection, Role role)
        : core::socket::stream::SocketContext(socketConnection)
        , role(role) {
    }

    void EchoSocketContext::onConnected() {
        const char* roleName = role == Role::CLIENT ? "client" : "server";
        const auto contextLog = log();

        contextLog.info("Echo {} context attached", roleName);

        if (role == Role::CLIENT) {
            contextLog.info("Echo client: sending initial greeting: '{}'", "Hello peer! Nice to see you!!!");
            sendToPeer("Hello peer! Nice to see you!!!");
        }
    }

    void EchoSocketContext::onDisconnected() {
        const char* roleName = role == Role::CLIENT ? "client" : "server";

        log().info(
            "Echo {} context detached: {}",
            roleName,
            getDetachReason() == DetachReason::ContextSwitch ? "context switch" : "connection close");
    }

    bool EchoSocketContext::onSignal([[maybe_unused]] int signum) {
        return true;
    }

    std::size_t EchoSocketContext::onReceivedFromPeer() {
        char chunk[4096];
        const std::size_t chunklen = readFromPeer(chunk, sizeof(chunk));

        if (chunklen > 0) {
            const char* roleName = role == Role::CLIENT ? "client" : "server";

            log().info("Echo {}: data to reflect: {}", roleName, std::string(chunk, chunklen));
            sendToPeer(chunk, chunklen);
        }

        return chunklen;
    }

    core::socket::stream::SocketContext*
    EchoServerSocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        return new EchoSocketContext(socketConnection, EchoSocketContext::Role::SERVER);
    }

    core::socket::stream::SocketContext*
    EchoClientSocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        return new EchoSocketContext(socketConnection, EchoSocketContext::Role::CLIENT);
    }

} // namespace echo
