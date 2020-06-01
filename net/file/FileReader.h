#ifndef FILEREADER_H
#define FILEREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "File.h"
#include "Reader.h"


class FileReader
    : public Reader
    , virtual public File {
protected:
    FileReader(int fd, const std::function<void(char* data, int len)>& junkRead, const std::function<void(int err)>& onError);

public:
    static FileReader* read(std::string path, const std::function<void(char* data, int len)>& junkRead,
                            const std::function<void(int err)>& onError);

    void stop();

    virtual void readEvent();

    void writerGone();

private:
    virtual void unmanaged();

protected:
    std::function<void(char* data, int len)> junkRead;

    bool writerOK;
};

#endif // FILEREADER_H
