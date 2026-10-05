set(stage "${SNODEC_BUILD_DIR}/staged-installed-consumer")
set(prefix "${stage}/prefix")
set(libdir "${prefix}/${SNODEC_INSTALL_LIBDIR}")

# Execute consumers against this temporary installation, not a system copy.
set(ENV{LD_LIBRARY_PATH} "${libdir}:${libdir}/snode.c/web/http")

file(REMOVE_RECURSE "${stage}")
file(MAKE_DIRECTORY "${stage}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "DESTDIR=${stage}" "${CMAKE_COMMAND}"
            --install "${SNODEC_BUILD_DIR}" --prefix /prefix
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_output
    ERROR_VARIABLE install_error
)
if(NOT install_result EQUAL 0)
    message(
        FATAL_ERROR "staged install failed\n${install_output}\n${install_error}"
    )
endif()

foreach(private_header IN
        ITEMS core/EventLoop.h core/EventMultiplexer.h
              core/DescriptorEventPublisher.h core/TimerEventPublisher.h
)
    if(EXISTS "${prefix}/include/snode.c/${private_header}")
        message(FATAL_ERROR "private header installed: ${private_header}")
    endif()
endforeach()

set(snodec_config_dir "${libdir}/cmake/snodec")
if(NOT EXISTS "${snodec_config_dir}/snodecConfig.cmake")
    message(
        FATAL_ERROR
            "installed snodecConfig.cmake missing in ${snodec_config_dir}"
    )
endif()

set(consumer_build "${stage}/consumer-build")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}/installed-consumer" -B
        "${consumer_build}" "-Dsnodec_DIR=${snodec_config_dir}"
        "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DCMAKE_CXX_FLAGS=${SNODEC_CXX_FLAGS}"
        "-DCMAKE_EXE_LINKER_FLAGS=${SNODEC_LINKER_FLAGS}"
        "-DCMAKE_FIND_ROOT_PATH=${SNODEC_FIND_ROOT_PATH}"
        -DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE
        -DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=TRUE
        -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE
        -DCMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=TRUE
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_output
    ERROR_VARIABLE configure_error
)
if(NOT configure_result EQUAL 0)
    message(
        FATAL_ERROR
            "installed consumer configure failed\n${configure_output}\n${configure_error}"
    )
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${consumer_build}"
    RESULT_VARIABLE build_result
    OUTPUT_VARIABLE build_output
    ERROR_VARIABLE build_error
)
if(NOT build_result EQUAL 0)
    message(
        FATAL_ERROR
            "installed consumer build failed\n${build_output}\n${build_error}"
    )
endif()

execute_process(
    COMMAND ${SNODEC_EMULATOR} "${consumer_build}/consumer"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE run_output
    ERROR_VARIABLE run_error
)
if(NOT run_result EQUAL 0)
    message(
        FATAL_ERROR
            "installed consumer execution failed\n${run_output}\n${run_error}"
    )
endif()
