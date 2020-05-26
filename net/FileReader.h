#ifndef FILEREADER_H
#define FILEREADER_H

<<<<<<< HEAD
#include <string>
#include <functional>
=======
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */
>>>>>>> MatthiasKoettritsch

#include "Reader.h"
#include "Descriptor.h"

<<<<<<< HEAD
class FileReader : public Reader,  public Descriptor
{
private:
    FileReader(int fd);
    
public:
    static void read(std::string path, std::function<void (unsigned char* data, int len)> junkRead, std::function<void (int err)> error);

    virtual void readEvent();
    
private:
    std::function<void (unsigned char* data, int len)> junkRead;
    std::function<void (int err)> error;
=======

class FileReader : public Reader
{
protected:
	FileReader (int fd, const std::function<void (char *data, int len)> &chunkRead, const std::function<void (int err)> &onError);

public:
	static FileReader *
	read (std::string path, const std::function<void (char *data, int len)> &chunkRead, const std::function<void (int err)> &onError);
	
	void stop ();
	
	virtual void readEvent ();

protected:
	std::function<void (char *data, int len)> chunkRead;
	
>>>>>>> MatthiasKoettritsch
};

#endif // FILEREADER_H
