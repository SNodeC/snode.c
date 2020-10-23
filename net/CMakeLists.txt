cmake_minimum_required(VERSION 3.0)

find_package(OpenSSL REQUIRED)

add_library(
  net SHARED
  AcceptEventReceiver.cpp
  ConnectEventReceiver.cpp
  Descriptor.cpp
  EventLoop.cpp
  FdSet.cpp
  OutOfBandEventReceiver.cpp
  ReadEventReceiver.cpp
  TimerEventDispatcher.cpp
  WriteEventReceiver.cpp
  file/File.cpp
  file/FileReader.cpp
  socket/ip/address/ipv4/InetAddress.cpp
  socket/ip/address/ipv6/InetAddress.cpp
  socket/ip/tcp/ipv4/Socket.cpp
  socket/ip/tcp/ipv6/Socket.cpp
  socket/sock_stream/tls/ssl_utils.cpp
  socket/sock_stream/tls/TLSHandshake.cpp
  timer/Timer.cpp)

target_link_libraries(net LINK_PUBLIC logger utils ${OPENSSL_LIBRARIES})

set_target_properties(
  net
  PROPERTIES CXX_STANDARD 20
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF
             SOVERSION 1)

target_compile_options(net PRIVATE --warn-all)

target_include_directories(net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                      ${OPENSSL_INCLUDE_DIR})

install(TARGETS net LIBRARY DESTINATION lib NAMELINK_COMPONENT libnet.so.1)
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
  DESTINATION include/snode.c/net
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "cmake" EXCLUDE)

if(CHECK_INCLUDES)
  set_property(TARGET net PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                   ${iwyu_path_and_options})
endif(CHECK_INCLUDES)
