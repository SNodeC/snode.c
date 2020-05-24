#include "SocketConnectionBase.h"

#include "Multiplexer.h"
#include "SocketServerInterface.h"
#include "SocketReader.h"
#include "SocketWriter.h"
#include "SSLSocketReader.h"
#include "SSLSocketWriter.h"
#include "FileReader.h"


template<typename R, typename W>
SocketConnectionBase<R, W>::SocketConnectionBase(int csFd,
                     SocketServerInterface* serverSocket,
                     const std::function<void (SocketConnectionInterface* cs, const char* junk, ssize_t n)>& readProcessor,
                     const std::function<void (int errnum)>& onReadError,
                     const std::function<void (int errnum)>& onWriteError
) :
R(readProcessor, [&] (int errnum) -> void {
    onReadError(errnum);
}),
W([&] (int errnum) -> void {
    if (fileReader) {
        fileReader->stop();
        fileReader = 0;
    }
    onWriteError(errnum);
}),
serverSocket(serverSocket),
fileReader(0) {
}

template<typename R, typename W>
SocketConnectionBase<R, W>::~SocketConnectionBase() {
    serverSocket->disconnect(this);
}

template<typename R, typename W>
InetAddress& SocketConnectionBase<R, W>::getRemoteAddress() {
    return remoteAddress;
}

template<typename R, typename W>
void SocketConnectionBase<R, W>::setRemoteAddress(const InetAddress& remoteAddress) {
    this->remoteAddress = remoteAddress;
}

template<typename R, typename W>
void SocketConnectionBase<R, W>::send(const char* puffer, int size) {
    Writer::writePuffer.append(puffer, size);
    Multiplexer::instance().getWriteManager().manageSocket(this);
}

template<typename R, typename W>
void SocketConnectionBase<R, W>::send(const std::string& junk) {
    Writer::writePuffer += junk;
    Multiplexer::instance().getWriteManager().manageSocket(this);
}

template<typename R, typename W>
void SocketConnectionBase<R, W>::sendFile(const std::string& file, const std::function<void (int ret)>& onError) {
    fileReader = FileReader::read(file,
                                  [this] (char* data, int length) -> void {
                                      if (length > 0) {
                                          this->SocketConnectionBase<R, W>::send(data, length);
                                      }
                                      fileReader = 0;
                                  },
                                  [this, onError] (int err) -> void {
                                      if (onError) {
                                          onError(err);
                                      }
                                      if (err) {
                                          this->end();
                                      }
                                  });
}

template<typename R, typename W>
void SocketConnectionBase<R, W>::end() {
    Multiplexer::instance().getReadManager().unmanageSocket(this);
}


template class SocketConnectionBase<SocketReader, SocketWriter>;
template class SocketConnectionBase<SSLSocketReader, SSLSocketWriter>;
