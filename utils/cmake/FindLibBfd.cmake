# * Try to find libbfd Once done this will define
#
# LIBBFD_FOUND - system has libbfd LIBBFD_INCLUDE_DIRS - the libbfd include
# directory LIBBFD_LIBRARIES - Link these to use libbfd LIBBFD_DEFINITIONS -
# Compiler switches required for using libbfd LIBIBERTY_LIBRARIES - libiberty
# static library (for static compilation) LIBZ_LIBRARIES - libz static library
# (for static compilation)

# if (LIBBFD_LIBRARIES AND LIBBFD_INCLUDE_DIRS) set (LibBpf_FIND_QUIETLY TRUE)
# endif (LIBBFD_LIBRARIES AND LIBBFD_INCLUDE_DIRS)

find_path(
    LIBBFD_INCLUDE_DIRS
    NAMES bfd.h
    PATHS ENV CPATH
)

find_library(
    LIBBFD_LIBRARIES
    NAMES bfd
    PATHS ENV LIBRARY_PATH ENV LD_LIBRARY_PATH
)

# libbfd.so is statically linked with libiberty.a but libbfd.a is not. So if we
# do a static bpftrace build, we must link in libiberty.a
find_library(
    LIBIBERTY_LIBRARIES
    NAMES libiberty.a
    PATHS ENV LIBRARY_PATH ENV LD_LIBRARY_PATH
)

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set LIBBFD_FOUND to TRUE if all
# listed variables are TRUE
find_package_handle_standard_args(
    LibBfd "Please install the libbfd development package" LIBBFD_LIBRARIES
    LIBBFD_INCLUDE_DIRS
)

mark_as_advanced(LIBBFD_INCLUDE_DIRS LIBBFD_LIBRARIES)
