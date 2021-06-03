cmake_minimum_required(VERSION 3.0)

find_package(OpenSSL REQUIRED)
pkg_check_modules(BLUEZ REQUIRED bluez)

find_package(range-v3 REQUIRED)

add_library(
    net SHARED
    AcceptEventReceiver.cpp
    ConnectEventReceiver.cpp
    Descriptor.cpp
    DescriptorEventDispatcher.cpp
    DescriptorEventReceiver.cpp
    EventLoop.cpp
    FdSet.cpp
    ExceptionalConditionEventReceiver.cpp
    ReadEventReceiver.cpp
    SNodeC.cpp
    TimerEventDispatcher.cpp
    WriteEventReceiver.cpp
    file/File.cpp
    file/FileReader.cpp
    pipe/Source.cpp
    pipe/Sink.cpp
    pipe/Pipe.cpp
    pipe/PipeSink.cpp
    pipe/PipeSource.cpp
    socket/bluetooth/address/L2CapAddress.cpp
    socket/bluetooth/address/RfCommAddress.cpp
    socket/bluetooth/l2cap/Socket.cpp
    socket/bluetooth/rfcomm/Socket.cpp
    socket/ip/address/ipv4/InetAddress.cpp
    socket/ip/address/ipv6/InetAddress.cpp
    socket/ip/tcp/ipv4/Socket.cpp
    socket/ip/tcp/ipv6/Socket.cpp
    socket/stream/SocketConnectionBase.cpp
    socket/stream/SocketContext.cpp
    socket/stream/tls/ssl_utils.cpp
    socket/stream/tls/TLSHandshake.cpp
    system/netdb.cpp
    system/select.cpp
    system/signal.cpp
    system/socket.cpp
    system/ssl.cpp
    system/time.cpp
    system/unistd.cpp
    timer/Timer.cpp
)

target_link_libraries(
    net LINK_PUBLIC utils logger ${OPENSSL_LIBRARIES} ${BLUEZ_LIBRARIES}
)

set_target_properties(
    net
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

target_compile_options(net PRIVATE --warn-all)

target_include_directories(
    net PUBLIC ${PROJECT_SOURCE_DIR} ${OPENSSL_INCLUDE_DIRS}
               ${BLUEZ_INCLUDE_DIRS}
)

install(TARGETS net LIBRARY DESTINATION lib NAMELINK_COMPONENT libnet.so.1)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/net
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

if(CHECK_INCLUDES)
    set_property(
        TARGET net PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
