cmake_minimum_required(VERSION 3.20)
project(openwrt_export_checks NONE)
enable_testing()
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
add_test(NAME OpenWrtDevelopmentExport
         COMMAND "${Python3_EXECUTABLE}"
                 "${CMAKE_CURRENT_SOURCE_DIR}/export.py"
)
set_tests_properties(
    OpenWrtDevelopmentExport PROPERTIES ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1"
)

add_test(NAME OpenWrtPackageArchitecture COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/architecture.py")

add_test(NAME NativeBuildParallelism COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/native.py")
