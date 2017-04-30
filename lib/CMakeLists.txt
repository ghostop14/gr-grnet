# Copyright 2011,2012,2016 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

set (CMAKE_CXX_STANDARD 11)

IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
	MESSAGE(STATUS "building for debug...")
ELSE(CMAKE_BUILD_TYPE MATCHES DEBUG)
	MESSAGE(STATUS "Release mode: increasing compiler optimizations...")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)

########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

list(APPEND grnet_sources
    tcp_sink_impl.cc
    tcp_source_impl.cc
    udp_sink_impl.cc
    udp_source_impl.cc
)

set(grnet_sources "${grnet_sources}" PARENT_SCOPE)
if(NOT grnet_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT grnet_sources)

add_library(gnuradio-grnet SHARED ${grnet_sources})
target_link_libraries(gnuradio-grnet ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES} z)
set_target_properties(gnuradio-grnet PROPERTIES DEFINE_SYMBOL "gnuradio_grnet_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-grnet PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
include(GrMiscUtils)
GR_LIBRARY_FOO(gnuradio-grnet RUNTIME_COMPONENT "grnet_runtime" DEVEL_COMPONENT "grnet_devel")

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_grnet_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_grnet.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_grnet.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_tcp_sink.cc
)

add_executable(test-grnet ${test_grnet_sources})

target_link_libraries(
  test-grnet
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  gnuradio-grnet
)

GR_ADD_TEST(test_grnet test-grnet)

########################################################################
# Build test-netsink
########################################################################
list(APPEND test_netsink_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_netsink.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_sink_impl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/udp_sink_impl.cc
)

add_executable(test-netsink ${test_netsink_sources})


target_link_libraries(
  test-netsink
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  boost_thread
  pthread
  z
  gnuradio-grnet
)

install(TARGETS test-netsink DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" RUNTIME)

########################################################################
# Build test-netsource
########################################################################
list(APPEND test_netsource_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_netsource.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_source_impl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/udp_source_impl.cc
)

add_executable(test-netsource ${test_netsource_sources})


target_link_libraries(
  test-netsource
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  boost_thread
  pthread
  z
  gnuradio-grnet
)

install(TARGETS test-netsource DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" RUNTIME)

########################################################################
# Print summary
########################################################################
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building for version: ${VERSION} / ${LIBVER}")

