find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(ECHO_TEST_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/echo_tests.py")

add_test(
    NAME echo.config-surface
    COMMAND
        "${Python3_EXECUTABLE}" "${ECHO_TEST_DRIVER}" config
        --server "$<TARGET_FILE:echoserver>"
        --client "$<TARGET_FILE:echoclient>"
)

add_test(
    NAME echo.server-external-peer
    COMMAND
        "${Python3_EXECUTABLE}" "${ECHO_TEST_DRIVER}" server
        --server "$<TARGET_FILE:echoserver>"
)

add_test(
    NAME echo.client-external-peer
    COMMAND
        "${Python3_EXECUTABLE}" "${ECHO_TEST_DRIVER}" client
        --client "$<TARGET_FILE:echoclient>"
)

add_test(
    NAME echo.pair-smoke
    COMMAND
        "${Python3_EXECUTABLE}" "${ECHO_TEST_DRIVER}" pair
        --server "$<TARGET_FILE:echoserver>"
        --client "$<TARGET_FILE:echoclient>"
)

set_tests_properties(
    echo.config-surface
    PROPERTIES
        LABELS "example;echo;external;configuration"
        TIMEOUT 10
)

set_tests_properties(
    echo.server-external-peer
    PROPERTIES
        LABELS "example;echo;external;server;payload"
        TIMEOUT 10
)

set_tests_properties(
    echo.client-external-peer
    PROPERTIES
        LABELS "example;echo;external;client;payload"
        TIMEOUT 10
)

set_tests_properties(
    echo.pair-smoke
    PROPERTIES
        LABELS "example;echo;external;pair;smoke"
        TIMEOUT 10
)
