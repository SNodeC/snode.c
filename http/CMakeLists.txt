cmake_minimum_required(VERSION 3.0)

# storage of custom FindLibMagic.cmake
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

find_package(LibMagic REQUIRED)

add_library(http SHARED WebApp.cpp HTTPContext.cpp HTTPStatusCodes.cpp MimeTypes.cpp Request.cpp Response.cpp httputils.cpp Router.cpp)

target_compile_options(http PRIVATE -fno-elide-constructors)

set_target_properties(http PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
)

target_include_directories(http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LIBMAGIC_INCLUDEs})
target_link_libraries(http LINK_PUBLIC net ${LIBMAGIC_LIBRARIES})
if(CHECK_INCLUDES)
    set_property(TARGET http PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif(CHECK_INCLUDES)
