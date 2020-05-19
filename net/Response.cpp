#include "Response.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <iostream>

#include "AcceptedSocket.h"


void Response::header() {
    acceptedSocket->writeLn("HTTP/1.1 200 OK");
    acceptedSocket->writeLn("Content-Type: text/html; charset=utf-8");
    acceptedSocket->writeLn("Connection: Closed");
    acceptedSocket->writeLn();
}


void Response::send(const std::string& text) {
    acceptedSocket->writeLn("HTTP/1.1 200 OK");
    acceptedSocket->writeLn("Content-Type: text/html; charset=iso-8859-1");
    acceptedSocket->writeLn("Content-Length: " + std::to_string(text.length()));
    acceptedSocket->writeLn("Connection: Closed");
    acceptedSocket->writeLn();
    acceptedSocket->write(text);
}


void Response::send(const char* buffer, int n) {
    acceptedSocket->write(buffer, n);
}


void Response::end() {
    acceptedSocket->close();
}
