#include "Response.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <iostream>

#include "ConnectedSocket.h"


void Response::send(const std::string& text) {
    connectedSocket->writeLn("HTTP/1.1 200 OK");
    connectedSocket->writeLn("Content-Type: text/html; charset=iso-8859-1");
    connectedSocket->writeLn("Content-Length: " + std::to_string(text.length()));
    connectedSocket->writeLn("Connection: Closed");
    connectedSocket->writeLn();
    connectedSocket->write(text);
//    connectedSocket->close();
}

void Response::end() {
    connectedSocket->writeLn("HTTP/1.1 200 OK");
    connectedSocket->writeLn("Connection: Closed");
    connectedSocket->writeLn();
    connectedSocket->close();
}
