cmake_minimum_required(VERSION 3.0)

# find_package(Backward) find_package(LibBfd)

add_library(utils SHARED Timeval.cpp instrumentation.cpp base64.cpp
)# stacktrace.cpp)

add_library(utils::utils ALIAS utils)

set_target_properties(
    utils
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

target_include_directories(
    utils
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(utils PUBLIC) # Backward::Backward ${LIBBFD_LIBRARIES})

install(
    TARGETS utils
    EXPORT utilsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/utils
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT utilsTargets
    FILE utilsTargets.cmake
    NAMESPACE utils::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/utils
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/utilsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/utils
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/utilsConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/utils
)

if(CHECK_INCLUDES)
    set_property(
        TARGET utils PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
