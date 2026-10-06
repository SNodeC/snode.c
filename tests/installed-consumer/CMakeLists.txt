cmake_minimum_required(VERSION 3.18)

project(SNodeCPackageInstalledConsumer LANGUAGES CXX)

include(CheckPIESupported)
check_pie_supported()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(
    snodec CONFIG REQUIRED
    COMPONENTS core net-in-stream-legacy net-un-stream-legacy
               http-server-express-legacy-in
)

add_executable(consumer main.cpp)
target_compile_features(consumer PRIVATE cxx_std_20)
target_link_libraries(
    consumer
    PRIVATE snodec::core snodec::net-in-stream-legacy
            snodec::net-un-stream-legacy snodec::http-server-express-legacy-in
)
