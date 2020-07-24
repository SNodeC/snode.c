#include <iostream>

#include "Multiplexer.h"
#include "socket/legacy/SocketClient.h"


int main(int argc, char* argv[]) {
    Multiplexer::init(argc, argv);
    
    legacy::SocketClient client(
        [](legacy::SocketConnection* connectedSocket) -> void { // onConnect
            std::cout << "OnConnect" << std::endl;
            connectedSocket->enqueue("GET /index.html HTTP/1.1\r\nConnection:keep-alive\r\n\r\n");
        },
        [](legacy::SocketConnection* connectedSocket) -> void { // onDisconnect
            std::cout << "OnDisConnect" << std::endl;
            Multiplexer::stop();
        },
        [](legacy::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
            std::cout << "OnRead" << std::endl;
            char *buf = new char[junkSize + 1];
            memcpy(buf, junk, junkSize);
            buf[junkSize] = 0;
            std::cout << "------------ begin data" << std::endl;
            std::cout << buf;
            std::cout << "------------ end data" << std::endl;
            delete[] buf;
        },
        [](legacy::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
            std::cout << "OnReadError: " << errnum << std::endl;
        },
        [](legacy::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
            std::cout << "OnWriteError: " << errnum << std::endl;
        });

    client.connect("localhost", 8080, [] (int err) -> void {
        if (err) {
            std::cout << "Connect Error: " << strerror(err) << std::endl;
            exit(-1);
        }
    });
    
    Multiplexer::start();
    
    return 0;
}
