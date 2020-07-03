cmake_minimum_required(VERSION 3.0)

find_package(OpenSSL REQUIRED)

add_library(net SHARED
    Descriptor.cpp
    Exception.cpp
    ManagedExceptions.cpp
    ManagedReader.cpp
    ManagedTimer.cpp
    ManagedWriter.cpp
    Multiplexer.cpp
    file/File.cpp
    file/FileReader.cpp
    socket/ClientSocket.cpp
    socket/InetAddress.cpp
    socket/Socket.cpp
    socket/SocketReader.cpp
    socket/SocketWriter.cpp
    socket/legacy/Socket.cpp
    socket/legacy/SocketReader.cpp
    socket/legacy/SocketWriter.cpp
    socket/legacy/SocketServer.cpp
    socket/tls/Socket.cpp
    socket/tls/SocketReader.cpp
    socket/tls/SocketWriter.cpp
    socket/tls/SocketServer.cpp
    timer/Timer.cpp
)

target_link_libraries(net LINK_PUBLIC logger ${OPENSSL_LIBRARIES})

set_target_properties(net PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            SOVERSION 1
)

FILE(GLOB includes "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

set_target_properties(net PROPERTIES PUBLIC_HEADER "${includes}")

target_include_directories (net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OPENSSL_INCLUDE_DIR})

install(TARGETS net LIBRARY DESTINATION lib)

if(CHECK_INCLUDES)
    set_property(TARGET net PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif(CHECK_INCLUDES)
