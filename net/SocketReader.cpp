#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_JUNKSIZE 16384

#include "SocketReader.h"
#include "Multiplexer.h"

#include "ConnectedSocket.h"


void SocketReader::readEvent ()
{
	static char chunk[MAX_JUNKSIZE];
	
	std::size_t ret = recv(this->getFd(), chunk, MAX_JUNKSIZE, 0);
	
	if (ret > 0)
	{
		readProcessor(dynamic_cast<ConnectedSocket *>(this), chunk, ret);
	} else if (ret == 0)
	{
		onError(0);
		Multiplexer::instance().getReadManager().unmanageSocket(this);
	} else
	{
		onError(errno);
		Multiplexer::instance().getReadManager().unmanageSocket(this);
	}
}

