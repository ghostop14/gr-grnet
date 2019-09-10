INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_GRNET grnet)

FIND_PATH(
    GRNET_INCLUDE_DIRS
    NAMES grnet/api.h
    HINTS $ENV{GRNET_DIR}/include
        ${PC_GRNET_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GRNET_LIBRARIES
    NAMES gnuradio-grnet
    HINTS $ENV{GRNET_DIR}/lib
        ${PC_GRNET_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/grnetTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GRNET DEFAULT_MSG GRNET_LIBRARIES GRNET_INCLUDE_DIRS)
MARK_AS_ADVANCED(GRNET_LIBRARIES GRNET_INCLUDE_DIRS)
