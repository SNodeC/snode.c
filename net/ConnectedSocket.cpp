#include <iostream>
#include <string.h>

#include "ConnectedSocket.h"
#include "SocketMultiplexer.h"
#include "ServerSocket.h"
#include "FileReader.h"


ConnectedSocket::ConnectedSocket(int csFd, Server* ss) : Socket(csFd), serverSocket(ss) {
}


ConnectedSocket::~ConnectedSocket() {
}


InetAddress& ConnectedSocket::getRemoteAddress() {
    return remoteAddress;
}


void ConnectedSocket::setRemoteAddress(const InetAddress& remoteAddress) {
    this->remoteAddress = remoteAddress;
}


void ConnectedSocket::send(const char* puffer, int size) {
    writePuffer.append(puffer, size);
    SocketMultiplexer::instance().getWriteManager().manageSocket(this);
}


void ConnectedSocket::send(const std::string& junk) {
    writePuffer += junk;
    SocketMultiplexer::instance().getWriteManager().manageSocket(this);
}


void ConnectedSocket::sendFile(const std::string& file) {
    FileReader::read(file,
                     [&] (char* data, int length) -> void {
                         if (length > 0) {
                             this->ConnectedSocket::send(data, length);
                         }
                     },
                     [&] (int err) -> void {
                         std::cout << "Error: " << strerror(err) << std::endl;
                         this->end();
                     });
}


void ConnectedSocket::end() {
    SocketMultiplexer::instance().getReadManager().unmanageSocket(this);
}


void ConnectedSocket::clearReadPuffer() {
    readPuffer.clear();
}


void ConnectedSocket::readEvent() {
    #define MAX_JUNKSIZE 4096
    char junk[MAX_JUNKSIZE];
    
    ssize_t ret = recv(this->getFd(), junk, MAX_JUNKSIZE, 0);
    
    if (ret > 0) {
        readPuffer.append(junk, ret);
    } else if (ret == 0) {
        std::cout << "EOF: " << this->getFd() << std::endl;
        SocketMultiplexer::instance().getReadManager().unmanageSocket(this);
    } else {
        std::cout << "Read: " << strerror(errno) << std::endl;
        SocketMultiplexer::instance().getReadManager().unmanageSocket(this);
    }
}


void ConnectedSocket::writeEvent() {
    ssize_t ret = ::send(this->getFd(), writePuffer.c_str(), (writePuffer.size() < 4096) ? writePuffer.size() : 4096, MSG_DONTWAIT | MSG_NOSIGNAL);
    
    if (ret >= 0) {
        writePuffer.erase(0, ret);
        if (writePuffer.empty()) {
            SocketMultiplexer::instance().getWriteManager().unmanageSocket(this);
        }
    } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        std::cout << "Write: " << strerror(errno) << std::endl;
        SocketMultiplexer::instance().getWriteManager().unmanageSocket(this);
    }
}
