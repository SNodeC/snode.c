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
    socket/ipv4/InetAddress.cpp
    socket/ipv4/InetAddress.h
    socket/ipv4/tcp/Socket.cpp
    socket/ipv4/tcp/Socket.h
    socket/ipv4/tcp/legacy/SocketClient.h
    socket/ipv4/tcp/legacy/SocketServer.h
    socket/ipv4/tcp/tls/SocketClient.h
    socket/ipv4/tcp/tls/SocketServer.h
    socket/ipv6/InetAddress.cpp
    socket/ipv6/InetAddress.h
    socket/ipv6/tcp/Socket.cpp
    socket/ipv6/tcp/Socket.h
    socket/ipv6/tcp/legacy/SocketClient.h
    socket/ipv6/tcp/legacy/SocketServer.h
    socket/ipv6/tcp/tls/SocketClient.h
    socket/ipv6/tcp/tls/SocketServer.h
    socket/Socket.h
    socket/SocketAddress.h
    socket/sock_stream/SocketClient.h
    socket/sock_stream/SocketConnection.h
    socket/sock_stream/SocketConnectionBase.h
    socket/sock_stream/SocketListener.h
    socket/sock_stream/SocketReader.h
    socket/sock_stream/SocketWriter.h
    socket/sock_stream/SocketServer.h
    socket/sock_stream/legacy/SocketClient.h
    socket/sock_stream/legacy/SocketConnection.h
    socket/sock_stream/legacy/SocketListener.h
    socket/sock_stream/legacy/SocketReader.h
    socket/sock_stream/legacy/SocketWriter.h
    socket/sock_stream/legacy/SocketServer.h
    socket/sock_stream/tls/ssl_utils.cpp
    socket/sock_stream/tls/ssl_utils.h
    socket/sock_stream/tls/SocketClient.h
    socket/sock_stream/tls/SocketConnection.h
    socket/sock_stream/tls/SocketListener.h
    socket/sock_stream/tls/SocketReader.h
    socket/sock_stream/tls/SocketWriter.h
    socket/sock_stream/tls/SocketServer.h
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
