cmake_minimum_required(VERSION 3.18)

if(LIBMARIADB_FOUND)

    get_target_property(HTTP_INSTALL_RPATH snodec::http-server HTTP_INSTALL_RPATH)

    get_target_property(HTTP_INSTALL_RPATH snodec::http-client HTTP_INSTALL_RPATH)

    add_subdirectory(authorization_server)
    add_subdirectory(client_app)
    add_subdirectory(resource_server)

endif(LIBMARIADB_FOUND)
