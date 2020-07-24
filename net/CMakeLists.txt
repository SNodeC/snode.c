cmake_minimum_required(VERSION 3.0)

find_package(OpenSSL REQUIRED)

add_library(net SHARED
    Descriptor.cpp
    Descriptor.h
    ManagedExceptions.cpp
    ManagedExceptions.h
    ManagedReader.cpp
    ManagedReader.h
    ManagedServer.cpp
    ManagedServer.h
    ManagedTimer.cpp
    ManagedTimer.h
    ManagedWriter.cpp
    ManagedWriter.h
    Multiplexer.cpp
    Multiplexer.h
    Exception.cpp
    Exception.h
    Reader.cpp
    Reader.h
    Server.cpp
    Server.h
    Writer.cpp
    Writer.h
    file/File.cpp
    file/File.h
    file/FileReader.cpp
    file/FileReader.h
    socket/InetAddress.cpp
    socket/InetAddress.h
    socket/Socket.cpp
    socket/Socket.h
    socket/SocketClient.h
    socket/SocketConnection.h
    socket/SocketConnectionBase.h
    socket/SocketReader.cpp
    socket/SocketReader.h
    socket/SocketWriter.cpp
    socket/SocketWriter.h
    socket/SocketServer.h
    socket/legacy/Socket.cpp
    socket/legacy/Socket.h
    socket/legacy/SocketClient.cpp
    socket/legacy/SocketClient.h
    socket/legacy/SocketConnection.h
    socket/legacy/SocketReader.cpp
    socket/legacy/SocketReader.h
    socket/legacy/SocketWriter.cpp
    socket/legacy/SocketWriter.h
    socket/legacy/SocketServer.cpp
    socket/legacy/SocketServer.h
    socket/tls/Socket.cpp
    socket/tls/Socket.h
    socket/tls/SocketConnection.h
    socket/tls/SocketReader.cpp
    socket/tls/SocketReader.h
    socket/tls/SocketWriter.cpp
    socket/tls/SocketWriter.h
    socket/tls/SocketServer.cpp
    socket/tls/SocketServer.h
    timer/Timer.cpp
    timer/Timer.h
)

target_link_libraries(net LINK_PUBLIC logger utils ${OPENSSL_LIBRARIES})

set_target_properties(net PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            SOVERSION 1
)

target_compile_options(net PRIVATE --warn-all)

FILE(GLOB includes "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

set_target_properties(net PROPERTIES PUBLIC_HEADER "${includes}")

target_include_directories (net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OPENSSL_INCLUDE_DIR})

install(TARGETS net LIBRARY DESTINATION lib)

if(CHECK_INCLUDES)
    set_property(TARGET net PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif(CHECK_INCLUDES)
