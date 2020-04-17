#include "MimeTypes.h"
#include <iostream>

magic_t MimeTypes::magic = MimeTypes::init();

magic_t MimeTypes::init() {
    magic_t magic = magic_open(MAGIC_MIME);
    
    if (magic_load(magic, NULL) != 0) {
        std::cerr << "cannot load magic database - " << magic_error(magic) << std::endl;
        magic_close(magic);
        exit(1);
    }
    
    return magic;
}
    

std::string MimeTypes::contentType(std::string file) {
    return magic_file(magic, (file.c_str()));
}
    
