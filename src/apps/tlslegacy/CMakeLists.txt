cmake_minimum_required(VERSION 3.18)

add_library(
    tlslegacysocketcontext STATIC TlsLegacySocketContext.cpp
                                  TlsLegacySocketContext.h
)
target_link_libraries(tlslegacysocketcontext PUBLIC core)

add_executable(tlslegacyserver tlslegacyserver.cpp)
target_link_libraries(
    tlslegacyserver PRIVATE tlslegacysocketcontext net-in-stream-tls
)
snodec_install_runtime_targets(
    COMPONENT apps
    TARGETS tlslegacyserver
)

add_executable(tlslegacyclient tlslegacyclient.cpp)
target_link_libraries(
    tlslegacyclient PRIVATE tlslegacysocketcontext net-in-stream-tls
)
snodec_install_runtime_targets(
    COMPONENT apps
    TARGETS tlslegacyclient
)
