#include <iostream>
#include <string.h>

#include "ConnectedSocket.h"
#include "Multiplexer.h"
#include "ServerSocket.h"
#include "FileReader.h"


ConnectedSocket::ConnectedSocket(int csFd, 
                                 ServerSocket* serverSocket, 
                                 std::function<void (ConnectedSocket* cs, const char* junk, ssize_t n)> readProcessor
                                ) 
: SocketReader(csFd, readProcessor), SocketWriter(csFd), serverSocket(serverSocket) {
}

ConnectedSocket::~ConnectedSocket() {
    serverSocket->disconnect(this);
}


InetAddress& ConnectedSocket::getRemoteAddress() {
    return remoteAddress;
}


void ConnectedSocket::setRemoteAddress(const InetAddress& remoteAddress) {
    this->remoteAddress = remoteAddress;
}


void ConnectedSocket::send(const char* puffer, int size) {
    writePuffer.append(puffer, size);
    Multiplexer::instance().getWriteManager().manageSocket(this);
}


void ConnectedSocket::send(const std::string& junk) {
    writePuffer += junk;
    Multiplexer::instance().getWriteManager().manageSocket(this);
}


void ConnectedSocket::sendFile(const std::string& file) {
    FileReader::read(file,
                     [this] (char* data, int length) -> void {
                         if (length > 0) {
                             this->ConnectedSocket::send(data, length);
                         }
                     },
                     [this] (int err) -> void {
                         std::cout << "Error: " << strerror(err) << std::endl;
                         this->end();
                     });
}


void ConnectedSocket::end() {
    Multiplexer::instance().getReadManager().unmanageSocket(this);
}
