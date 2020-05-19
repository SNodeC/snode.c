#ifndef CONNECTEDSOCKET_H
#define CONNECTEDSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "InetAddress.h"
#include "SocketWriter.h"
#include "SocketReader.h"


class FileReader;

class ServerSocket;

class ConnectedSocket : public SocketReader, public SocketWriter
{
public:
	ConnectedSocket (int csFd,
	                 ServerSocket *ss,
	                 const std::function<void (ConnectedSocket *cs, const char *chunk, std::size_t n)> &readProcessor,
	                 const std::function<void (int errnum)> &onReadError,
	                 const std::function<void (int errnum)> &onWriteError
	);
	
	virtual ~ConnectedSocket ();
	
	void setContext (void *context)
	{
		this->context = context;
	}
	
	void *getContext ()
	{
		return this->context;
	}
	
	virtual void send (const char *puffer, int size);
	
	virtual void send (const std::string &chunk);
	
	virtual void sendFile (const std::string &file, const std::function<void (int ret)> &onError);
	
	void end ();
	
	InetAddress &getRemoteAddress ();
	
	void setRemoteAddress (const InetAddress &remoteAddress);

protected:
	ServerSocket *serverSocket;
	void *context;
	
	InetAddress remoteAddress;

private:
	FileReader *fileReader;
};

#endif // CONNECTEDSOCKET_H
