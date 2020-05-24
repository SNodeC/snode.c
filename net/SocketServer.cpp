#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iostream>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketServer.h"
#include "Multiplexer.h"

template<typename T>
SocketServerBase<T>::SocketServerBase(const std::function<void (SocketConnectionInterface* cs)>& onConnect,
                                      const std::function<void (SocketConnectionInterface* cs)>& onDisconnect,
                                      const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcessor,
                                      const std::function<void (int errnum)>& onCsReadError,
                                      const std::function<void (int errnum)>& onCsWriteError)
    : SocketReader(), onConnect(onConnect), onDisconnect(onDisconnect), readProcessor(readProcessor), onCsReadError(onCsReadError), onCsWriteError(onCsWriteError)
{}


template<typename T>
void SocketServerBase<T>::listen(in_port_t port, int backlog, const std::function<void (int err)>& onError) {
    this->SocketReader::setOnError(onError);

    this->open([this, &port, &backlog, &onError] (int errnum) -> void {
        if (errnum > 0) {
            onError(errnum);
        } else {
            int sockopt = 1;

            if (setsockopt(this->getFd(), SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0) {
                onError(errno);
            } else {
                localAddress = InetAddress(port);
                this->bind(localAddress, [this, &backlog, &onError] (int errnum) -> void {
                    if (errnum > 0) {
                        onError(errnum);
                    } else {
                        this->Socket::listen(backlog, [this, &onError] (int errnum) -> void {
                            if (errnum == 0) {
                                Multiplexer::instance().getReadManager().manageSocket(this);
                            }
                            onError(errnum);
                        });
                    }
                });
            }
        }
    });
}


template<typename T>
void SocketServerBase<T>::readEvent() {
    struct sockaddr_in remoteAddress;
    socklen_t addrlen = sizeof(remoteAddress);

    int csFd = -1;

    csFd = ::accept(this->getFd(), (struct sockaddr*) &remoteAddress, &addrlen);

    if (csFd >= 0) {
        struct sockaddr_in localAddress;
        socklen_t addressLength = sizeof(localAddress);

        if (getsockname(csFd, (struct sockaddr*) &localAddress, &addressLength) == 0) {
            T* cs = new T(csFd, this, this->readProcessor, onCsReadError, onCsWriteError);

            cs->setRemoteAddress(remoteAddress);
            cs->setLocalAddress(localAddress);

            Multiplexer::instance().getReadManager().manageSocket(cs);
            onConnect(cs);
        } else {
            shutdown(csFd, SHUT_RDWR);
            close(csFd);
            onError(errno);
        }
    } else if (errno != EINTR) {
        onError(errno);
    }
}


template<typename T>
void SocketServerBase<T>::disconnect(SocketConnectionInterface* cs) {
    if (onDisconnect) {
        onDisconnect(cs);
    }
}


SSLSocketServer::SSLSocketServer(const std::function<void (SocketConnectionInterface* cs)>& onConnect,
                const std::function<void (SocketConnectionInterface* cs)>& onDisconnect,
                const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcessor,
                const std::function<void (int errnum)>& onCsReadError,
                const std::function<void (int errnum)>& onCsWriteError) :
    SocketServerBase<SSLSocketConnection>(
        [&] (SocketConnectionInterface* cs) {
            dynamic_cast<SSLSocketConnection*>(cs)->setCTX(ctx);
            this->onConnect(cs);
        }, onDisconnect, readProcessor, onCsReadError, onCsWriteError), onConnect(onConnect), ctx(0) {
}

SSLSocketServer* SSLSocketServer::instance(const std::function<void (SocketConnectionInterface* cs)>& onConnect,
                                           const std::function<void (SocketConnectionInterface* cs)>& onDisconnect,
                                           const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcessor,
                                           const std::function<void (int errnum)>& onCsReadError,
                                           const std::function<void (int errnum)>& onCsWriteError) {
    return new SSLSocketServer(onConnect, onDisconnect, readProcessor, onCsReadError, onCsWriteError);
}


SSLSocketServer::~SSLSocketServer() {
    if (ctx) {
        SSL_CTX_free(ctx);
    }
}


void SSLSocketServer::listen(in_port_t port, int backlog, const std::string& cert, const std::string& key, const std::string& password, const std::function<void (int err)>& onError) {
    SocketServerBase<SSLSocketConnection>::listen(port, backlog, 
        [onError, this, &cert, &key, &password] (int err) -> void {
            SSL_load_error_strings();
            SSL_library_init();
            OpenSSL_add_ssl_algorithms();
                
            ctx = SSL_CTX_new(TLS_server_method());
            if (!ctx) {
                ERR_print_errors_fp(stderr);
                exit(2);
            }
                
            if (SSL_CTX_use_certificate_file(ctx, cert.c_str(), SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                exit(3);
            }
            
            if (SSL_CTX_use_PrivateKey_file(ctx, key.c_str(), SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                exit(4);
            }
                
            if (!SSL_CTX_check_private_key(ctx)) {
                fprintf(stderr,"Private key does not match the certificate public key\n");
                exit(5);
            }
            
            onError(err);
        }
    );
}


void SSLSocketServer::readEvent() {
    SocketServerBase<SSLSocketConnection>::readEvent();
}


SocketServer::SocketServer(const std::function<void (SocketConnectionInterface* cs)>& onConnect,
                           const std::function<void (SocketConnectionInterface* cs)>& onDisconnect,
                           const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcessor,
                           const std::function<void (int errnum)>& onCsReadError,
                           const std::function<void (int errnum)>& onCsWriteError) :
    SocketServerBase<SocketConnection>(onConnect, onDisconnect, readProcessor, onCsReadError, onCsWriteError) {
}
                                 
SocketServer* SocketServer::instance(const std::function<void (SocketConnectionInterface* cs)>& onConnect,
                                     const std::function<void (SocketConnectionInterface* cs)>& onDisconnect,
                                     const std::function<void (SocketConnectionInterface* cs, const char*  junk, ssize_t n)>& readProcessor,
                                     const std::function<void (int errnum)>& onCsReadError,
                                     const std::function<void (int errnum)>& onCsWriteError) {
    return new SocketServer(onConnect, onDisconnect, readProcessor, onCsReadError, onCsWriteError);
}
