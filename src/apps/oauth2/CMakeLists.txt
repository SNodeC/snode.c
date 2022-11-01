cmake_minimum_required(VERSION 3.5)

if(MARIADB_FOUND)

    add_compile_options(
        -Werror
        -Wall
        -Wextra
        -Wno-psabi # needed for RaspberryPi
        -Wconversion
        -Wpedantic
        -Wconversion
        -Wuninitialized
        -Wunreachable-code
        -Wno-float-equal
        -Wno-missing-prototypes
        -Wno-undefined-func-template
        -Wno-implicit-int-float-conversion
        -pedantic-errors
        -fexec-charset=UTF-8
    )

    add_subdirectory(authorization_server)
    add_subdirectory(client_app)
    add_subdirectory(resource_server)

endif(MARIADB_FOUND)
