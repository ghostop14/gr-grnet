# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT grnet_sources)
    MESSAGE(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT grnet_sources)

########################################################################
# Check for pygccxml
########################################################################
GR_PYTHON_CHECK_MODULE_RAW(
    "pygccxml"
    "import pygccxml"
    PYGCCXML_FOUND
    )

include(GrPybind)

########################################################################
# Python Bindings
########################################################################

list(APPEND grnet_python_files
    ByteComplexDecimator_python.cc
    BytesToSC16_python.cc
    ComplexToInt16Bytes_python.cc
    ComplexToSC16_python.cc
    ComplexToSigned8_python.cc
    FifoBuffer_python.cc
    Int16BytesToComplex_python.cc
    IShortToSC16_python.cc
    PCAPUDPSource_python.cc
    SC16ToBytes_python.cc
    SC16ToComplex_python.cc
    SC16ToIShort_python.cc
    Signed8ToComplex_python.cc
    tcp_sink_python.cc
    udp_sink_python.cc
    udp_source_python.cc python_bindings.cc)

GR_PYBIND_MAKE_OOT(grnet 
   ../..
   gr::grnet
   "${grnet_python_files}")

install(TARGETS grnet_python DESTINATION ${GR_PYTHON_DIR}/grnet COMPONENT pythonapi)
