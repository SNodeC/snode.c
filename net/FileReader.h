#ifndef FILEREADER_H
#define FILEREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "File.h"
#include "Reader.h"
//#include "Descriptor.h"


class FileReader : public Reader, virtual public File
{
protected:
    FileReader(int fd, const std::function<void (char* data, int len)>& junkRead, const std::function<void (int err)>& onError);
    
public:
    static FileReader* read(std::string path, const std::function<void (char* data, int len)>& junkRead, const std::function<void (int err)>& onError);
    
    void stop();
    
    virtual void readEvent();
    
protected:
    std::function<void (char* data, int len)> junkRead;
    
};

#endif // FILEREADER_H
