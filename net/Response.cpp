#include "Response.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <iostream>

#include "AcceptedSocket.h"


void Response::send(const std::string& text) {
    acceptedSocket->writeLn("HTTP/1.1 200 OK");
    acceptedSocket->writeLn("Content-Type: text/html; charset=iso-8859-1");
    acceptedSocket->writeLn("Content-Length: " + std::to_string(text.length()));
    acceptedSocket->writeLn("Connection: Closed");
    acceptedSocket->writeLn();
    acceptedSocket->write(text);
    //    acceptedSocket->close();
}

void Response::end() {
    acceptedSocket->writeLn("HTTP/1.1 200 OK");
    acceptedSocket->writeLn("Connection: Closed");
    acceptedSocket->writeLn();
    acceptedSocket->close();
}
