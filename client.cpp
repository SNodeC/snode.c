#include <iostream>

#include "Multiplexer.h"
#include "socket/tls/SocketClient.h"
#include "socket/legacy/SocketClient.h"


tls::SocketClient tlsClient() {
    tls::SocketClient client(
        []([[maybe_unused]] tls::SocketConnection* connectedSocket) -> void { // onConnect
            std::cout << "OnConnect" << std::endl;
            connectedSocket->enqueue("GET /index.html HTTP/1.1\r\n\r\n"); //Connection:keep-alive\r\n\r\n");
        },
        []([[maybe_unused]] tls::SocketConnection* connectedSocket) -> void { // onDisconnect
            std::cout << "OnDisConnect" << std::endl;
        },
        []([[maybe_unused]] tls::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
            std::cout << "OnRead" << std::endl;
            char *buf = new char[junkSize + 1];
            memcpy(buf, junk, junkSize);
            buf[junkSize] = 0;
            std::cout << "------------ begin data" << std::endl;
            std::cout << buf;
            std::cout << "------------ end data" << std::endl;
            delete[] buf;
        },
        []([[maybe_unused]] tls::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
            std::cout << "OnReadError: " << errnum << std::endl;
        },
        []([[maybe_unused]] tls::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
            std::cout << "OnWriteError: " << errnum << std::endl;
        });

    client.connect("localhost", 8088, [] (int err) -> void {
        if (err) {
            std::cout << "Connect Error: " << strerror(err) << std::endl;
            Multiplexer::stop();
        } else {
            std::cout << "Connected" << std::endl;
        }
    });
    
    return client;
}

legacy::SocketClient legacyClient() {
    legacy::SocketClient legacyClient(
        []([[maybe_unused]] legacy::SocketConnection* connectedSocket) -> void { // onConnect
            std::cout << "OnConnect" << std::endl;
            connectedSocket->enqueue("GET /index.html HTTP/1.1\r\n\r\n"); //Connection:keep-alive\r\n\r\n");
        },
        []([[maybe_unused]] legacy::SocketConnection* connectedSocket) -> void { // onDisconnect
            std::cout << "OnDisConnect" << std::endl;
        },
        []([[maybe_unused]] legacy::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
            std::cout << "OnRead" << std::endl;
            char *buf = new char[junkSize + 1];
            memcpy(buf, junk, junkSize);
            buf[junkSize] = 0;
            std::cout << "------------ begin data" << std::endl;
            std::cout << buf;
            std::cout << "------------ end data" << std::endl;
            delete[] buf;
        },
        []([[maybe_unused]] legacy::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
            std::cout << "OnReadError: " << errnum << std::endl;
        },
        []([[maybe_unused]] legacy::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
            std::cout << "OnWriteError: " << errnum << std::endl;
        });
    
    legacyClient.connect("localhost", 8080, [] (int err) -> void {
        if (err) {
            std::cout << "Connect Error: " << strerror(err) << std::endl;
            Multiplexer::stop();
        } else {
            std::cout << "Connected" << std::endl;
        }
    });
    
    return legacyClient;
}


int main(int argc, char* argv[]) {
    Multiplexer::init(argc, argv);
    
    tls::SocketClient sc = tlsClient();
    legacy::SocketClient lc = legacyClient();
    
    Multiplexer::start();
}
