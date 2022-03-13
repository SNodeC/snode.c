cmake_minimum_required(VERSION 3.0)

find_package(PkgConfig REQUIRED)

pkg_check_modules(MARIADB mariadb)

add_executable(test_mariadb_client "test_mariadb_client.cpp" "MariaDBClient.cpp" "MariaDBExecutor.cpp" "MariaDBCommand.cpp" "command/MariaDBConnectCommand.cpp" "command/MariaDBQueryCommand.cpp")
target_link_libraries(test_mariadb_client PUBLIC snodec::core)
target_link_libraries(test_mariadb_client PUBLIC ${MARIADB_LIBRARIES})
target_include_directories(test_mariadb_client PUBLIC ${MARIADB_INCLUDE_DIRS})
add_compile_options(-pthread)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
