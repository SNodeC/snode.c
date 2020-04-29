cmake_minimum_required(VERSION 3.0)

add_library(net SHARED 
    Multiplexer.cpp 
    Descriptor.cpp 
    Socket.cpp 
    SocketReader.cpp
    SocketWriter.cpp
    ServerSocket.cpp
    ConnectedSocket.cpp
    ClientSocket.cpp
    TimerManager.cpp
    ReadManager.cpp
    WriteManager.cpp
    ExceptionManager.cpp
    InetAddress.cpp
    Timer.cpp 
    FileReader.cpp
)

target_link_libraries(net LINK_PUBLIC magic)

target_include_directories (net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
