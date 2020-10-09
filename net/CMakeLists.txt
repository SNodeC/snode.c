cmake_minimum_required(VERSION 3.0)

find_package(OpenSSL REQUIRED)

add_library(net SHARED
    Descriptor.cpp
    Descriptor.h
    OutOfBandEventDispatcher.cpp
    OutOfBandEventDispatcher.h
    ReadEventDispatcher.cpp
    ReadEventDispatcher.h
    AcceptEventDispatcher.cpp
    AcceptEventDispatcher.h
    TimerEventDispatcher.cpp
    TimerEventDispatcher.h
    WriteEventDispatcher.cpp
    WriteEventDispatcher.h
    EventDispatcher.h
    EventReceiver.h
    EventLoop.cpp
    EventLoop.h
    OutOfBandEventReceiver.cpp
    OutOfBandEventReceiver.h
    ReadEventReceiver.cpp
    ReadEventReceiver.h
    AcceptEventReceiver.cpp
    AcceptEventReceiver.h
    WriteEventReceiver.cpp
    WriteEventReceiver.h
    file/File.cpp
    file/File.h
    file/FileReader.cpp
    file/FileReader.h
    socket/InetAddress.cpp
    socket/InetAddress.h
    socket/Socket.cpp
    socket/Socket.h
    socket/tcp/Socket.cpp
    socket/tcp/Socket.h
    socket/tcp/SocketClient.h
    socket/tcp/SocketConnection.h
    socket/tcp/SocketConnectionBase.h
    socket/tcp/SocketListener.h
    socket/tcp/SocketReader.h
    socket/tcp/SocketWriter.h
    socket/tcp/SocketServer.h
    socket/tcp/legacy/Socket.h
    socket/tcp/legacy/SocketClient.h
    socket/tcp/legacy/SocketConnection.h
    socket/tcp/legacy/SocketListener.h
    socket/tcp/legacy/SocketReader.cpp
    socket/tcp/legacy/SocketReader.h
    socket/tcp/legacy/SocketWriter.cpp
    socket/tcp/legacy/SocketWriter.h
    socket/tcp/legacy/SocketServer.h
    socket/tcp/tls/ssl_utils.cpp
    socket/tcp/tls/ssl_utils.h
    socket/tcp/tls/Socket.cpp
    socket/tcp/tls/Socket.h
    socket/tcp/tls/SocketClient.cpp
    socket/tcp/tls/SocketClient.h
    socket/tcp/tls/SocketConnection.h
    socket/tcp/tls/SocketListener.cpp
    socket/tcp/tls/SocketListener.h
    socket/tcp/tls/SocketReader.cpp
    socket/tcp/tls/SocketReader.h
    socket/tcp/tls/SocketWriter.cpp
    socket/tcp/tls/SocketWriter.h
    socket/tcp/tls/SocketServer.h
    timer/IntervalTimer.h
    timer/SingleshotTimer.h
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

target_include_directories (net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OPENSSL_INCLUDE_DIR})

install(TARGETS net LIBRARY DESTINATION lib NAMELINK_COMPONENT libnet.so.1)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION include/snode.c/net FILES_MATCHING PATTERN "*.h" PATTERN "cmake" EXCLUDE)

if(CHECK_INCLUDES)
    set_property(TARGET net PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options})
endif(CHECK_INCLUDES)
