cmake_minimum_required(VERSION 3.0)

find_package(OpenSSL REQUIRED)

add_library(net SHARED 
    ClientSocket.cpp
    Descriptor.cpp
    File.cpp
    FileReader.cpp
    InetAddress.cpp
    ManagedExceptions.cpp
    ManagedReader.cpp
    ManagedTimer.cpp
    ManagedWriter.cpp
    Multiplexer.cpp
    Socket.cpp
    SocketConnectionBase.cpp
    legacy/SocketLegacyConnection.cpp
    legacy/SocketLegacy.cpp
    legacy/SocketLegacyReader.cpp
    legacy/SocketLegacyWriter.cpp
    legacy/SocketLegacyServer.cpp
    SocketServer.cpp
    SocketServerBase.cpp
    ssl/SocketSSLConnection.cpp
    ssl/SocketSSL.cpp
    ssl/SocketSSLReader.cpp
    ssl/SocketSSLWriter.cpp
    ssl/SocketSSLServer.cpp
    timer/Timer.cpp
)

target_link_libraries(net ${OPENSSL_LIBRARIES})

set_target_properties(net PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            SOVERSION 1
)

FILE(GLOB includes "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

set_target_properties(net PROPERTIES PUBLIC_HEADER "${includes}")

target_include_directories (net PUBLIC ssl legacy timer ${CMAKE_CURRENT_SOURCE_DIR} ${OPENSSL_INCLUDE_DIR})

install(TARGETS net LIBRARY DESTINATION lib)
#install(TARGETS net PUBLIC_HEADER DESTINATION include)

if(CHECK_INCLUDES)
    set_property(TARGET net PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif(CHECK_INCLUDES)
