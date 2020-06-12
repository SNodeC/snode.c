#ifndef MIMETYPES_H
#define MIMETYPES_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <magic.h>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class MimeTypes {
public:
    MimeTypes(const MimeTypes&) = delete;
    MimeTypes operator=(const MimeTypes&) = delete;

    ~MimeTypes();

    static std::string contentType(const std::string& file);

private:
    MimeTypes();

    static magic_t magic;

    static MimeTypes mimeTypes;

    static std::map<std::string, std::string> mimeType;
};

#endif // MIMETYPES_H
