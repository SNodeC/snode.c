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
    static FileReader* read(const std::string& path, const std::function<void(char* data, int len)>& junkRead,
                            const std::function<void(int err)>& onError);

    void stop();

    void readEvent() override;

private:
    void unmanaged() override;

    std::function<void(char* data, int len)> junkRead;

    bool stopped;
};

#endif // FILEREADER_H
