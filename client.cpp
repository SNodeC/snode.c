#include <iostream>

#include "Multiplexer.h"
#include "socket/tls/SocketClient.h"


tls::SocketClient client() {
    tls::SocketClient client(
        [](tls::SocketConnection* connectedSocket) -> void { // onConnect
            std::cout << "OnConnect" << std::endl;
            connectedSocket->enqueue("GET /index.html HTTP/1.1\r\nConnection:keep-alive\r\n\r\n");
        },
        [](tls::SocketConnection* connectedSocket) -> void { // onDisconnect
            std::cout << "OnDisConnect" << std::endl;
            Multiplexer::stop();
        },
        [](tls::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
            std::cout << "OnRead" << std::endl;
            char *buf = new char[junkSize + 1];
            memcpy(buf, junk, junkSize);
            buf[junkSize] = 0;
            std::cout << "------------ begin data" << std::endl;
            std::cout << buf;
            std::cout << "------------ end data" << std::endl;
            delete[] buf;
        },
        [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
            std::cout << "OnReadError: " << errnum << std::endl;
        },
        [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
            std::cout << "OnWriteError: " << errnum << std::endl;
        });

    client.connect("localhost", 8088, [&client] (int err) -> void {
        if (err) {
            std::cout << "Connect Error: " << strerror(err) << std::endl;
        }
    });
    
    return client;
}

int main(int argc, char* argv[]) {
    Multiplexer::init(argc, argv);
    
    tls::SocketClient sc = client();
    
    Multiplexer::start();
}
