#include "WriteManager.h"


int WriteManager::dispatch (fd_set &fdSet, int count)
{
	for (std::list<Writer *>::iterator it = descriptors.begin(); it != descriptors.end() && count > 0; ++it)
	{
		if (FD_ISSET((*it)->getFd(), &fdSet))
		{
			count--;
			(*it)->writeEvent();
		}
	}
	
	return count;
}
