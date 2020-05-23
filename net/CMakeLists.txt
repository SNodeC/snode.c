cmake_minimum_required(VERSION 3.0)

add_library(net SHARED 
    Multiplexer.cpp 
    Descriptor.cpp 
    SocketBase.cpp 
    Socket.cpp 
    SSLSocket.cpp
    SocketReader.cpp
    SocketWriter.cpp
    SSLSocketReader.cpp 
    SSLSocketWriter.cpp 
    ServerSocket.cpp
    ConnectedSocket.cpp
    SSLConnectedSocket.cpp 
    ClientSocket.cpp
    TimerManager.cpp
    ReadManager.cpp
    WriteManager.cpp
    ExceptionManager.cpp
    InetAddress.cpp
    Timer.cpp 
    FileReader.cpp
    StartUp.cpp
    File.cpp
)

target_include_directories (net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# TODO: Add support for STATIC also.
find_package(OpenSSL REQUIRED)

message("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message("OpenSSL libraries: ${OPENSSL_LIBRARIES}")

include_directories(${OPENSSL_INCLUDE_DIR})
list(APPEND LIB_LIST ${OPENSSL_LIBRARIES})

target_link_libraries(net ${OPENSSL_LIBRARIES})
        
#find_program(iwyu_path NAMES include-what-you-use iwyu)
#if(NOT iwyu_path)
#  message(FATAL_ERROR "Could not find the program include-what-you-use")
#endif()
#set_property(TARGET net PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
