#include "AcceptedSocket.h"

#include "ServerSocket.h"


AcceptedSocket::AcceptedSocket(int csFd, ServerSocket* ss) : ConnectedSocket(csFd), serverSocket(ss), request(this), response(this) {
}

void AcceptedSocket::ready() {
    serverSocket->process(request, response);
}
