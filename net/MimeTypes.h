#ifndef MIMETYPES_H
#define MIMETYPES_H

#include <magic.h>

#include <string>

class MimeTypes
{
public:
    static std::string contentType(std::string file);
    
    static magic_t init();
    
private:
    static magic_t magic;
};

#endif // MIMETYPES_H
