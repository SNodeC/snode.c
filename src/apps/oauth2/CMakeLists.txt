cmake_minimum_required(VERSION 3.5)

if(MARIADB_FOUND)

    add_subdirectory(authorization_server)
    add_subdirectory(client_app)
    add_subdirectory(resource_server)

endif(MARIADB_FOUND)
