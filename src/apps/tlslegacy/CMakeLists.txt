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
install(TARGETS tlslegacyserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                        COMPONENT apps
)

add_executable(tlslegacyclient tlslegacyclient.cpp)
target_link_libraries(
    tlslegacyclient PRIVATE tlslegacysocketcontext net-in-stream-tls
)
install(TARGETS tlslegacyclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                        COMPONENT apps
)
