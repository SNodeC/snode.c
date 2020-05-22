#ifndef MIMETYPES_H
#define MIMETYPES_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <magic.h>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class MimeTypes
{
public:
    static std::string contentType(std::string file);
    
private:
    static magic_t init();
    
    static magic_t magic;
    
    static std::map<std::string, std::string> mimeTypes;
};

#endif // MIMETYPES_H
