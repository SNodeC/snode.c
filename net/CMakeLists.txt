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
    SocketLegacyConnection.cpp
    SocketLegacy.cpp
    SocketLegacyReader.cpp
    SocketLegacyWriter.cpp
    SocketLegacyServer.cpp
    SocketServer.cpp
    SocketServerBase.cpp
    SocketSSLConnection.cpp
    SocketSSL.cpp
    SocketSSLReader.cpp
    SocketSSLWriter.cpp
    SocketSSLServer.cpp
    Timer.cpp
)

target_link_libraries(net ${OPENSSL_LIBRARIES})

set_target_properties(net PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            SOVERSION 1
)

FILE(GLOB includes "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

set_target_properties(net PROPERTIES PUBLIC_HEADER "${includes}")

target_include_directories (net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OPENSSL_INCLUDE_DIR})

install(TARGETS net LIBRARY DESTINATION lib)
#install(TARGETS net PUBLIC_HEADER DESTINATION include)

if(CHECK_INCLUDES)
    set_property(TARGET net PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif(CHECK_INCLUDES)
