#ifndef SOCKET_H
#define SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "InetAddress.h"
#include "Descriptor.h"


class Socket : virtual public Descriptor
{
public:
	virtual ~Socket ();
	
	void bind (InetAddress &localAddress, const std::function<void (int errnum)> &onError);
	
	InetAddress &getLocalAddress ();
	
	void setLocalAddress (const InetAddress &localAddress);

protected:
	Socket ();
	
	void open (const std::function<void (int errnum)> &onError);
	
	InetAddress localAddress;
};

#endif // SOCKET_H
