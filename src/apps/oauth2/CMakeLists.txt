cmake_minimum_required(VERSION 3.5)

if(MARIADB_FOUND)

    get_target_property(HTTP_SERVER_RPATH snodec::http-server HTTP_SERVER_RPATH)

    get_target_property(HTTP_CLIENT_RPATH snodec::http-client HTTP_CLIENT_RPATH)

    add_subdirectory(authorization_server)
    add_subdirectory(client_app)
    add_subdirectory(resource_server)

endif(MARIADB_FOUND)
