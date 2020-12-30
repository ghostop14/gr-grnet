/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <pybind11/pybind11.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

namespace py = pybind11;

// Headers for binding functions
/**************************************/
/* The following comment block is used for
/* gr_modtool to insert function prototypes
/* Please do not delete
/**************************************/
// BINDING_FUNCTION_PROTOTYPES(
    void bind_ByteComplexDecimator(py::module& m);
    void bind_BytesToSC16(py::module& m);
    void bind_ComplexToInt16Bytes(py::module& m);
    void bind_ComplexToSC16(py::module& m);
    void bind_ComplexToSigned8(py::module& m);
    void bind_FifoBuffer(py::module& m);
    void bind_Int16BytesToComplex(py::module& m);
    void bind_IShortToSC16(py::module& m);
    void bind_PCAPUDPSource(py::module& m);
    void bind_SC16ToBytes(py::module& m);
    void bind_SC16ToComplex(py::module& m);
    void bind_SC16ToIShort(py::module& m);
    void bind_Signed8ToComplex(py::module& m);
    void bind_tcp_sink(py::module& m);
    void bind_udp_sink(py::module& m);
    void bind_udp_source(py::module& m);
// ) END BINDING_FUNCTION_PROTOTYPES


// We need this hack because import_array() returns NULL
// for newer Python versions.
// This function is also necessary because it ensures access to the C API
// and removes a warning.
void* init_numpy()
{
    import_array();
    return NULL;
}

PYBIND11_MODULE(grnet_python, m)
{
    // Initialize the numpy C API
    // (otherwise we will see segmentation faults)
    init_numpy();

    // Allow access to base block methods
    py::module::import("gnuradio.gr");

    /**************************************/
    /* The following comment block is used for
    /* gr_modtool to insert binding function calls
    /* Please do not delete
    /**************************************/
    // BINDING_FUNCTION_CALLS(
    bind_ByteComplexDecimator(m);
    bind_BytesToSC16(m);
    bind_ComplexToInt16Bytes(m);
    bind_ComplexToSC16(m);
    bind_ComplexToSigned8(m);
    bind_FifoBuffer(m);
    bind_Int16BytesToComplex(m);
    bind_IShortToSC16(m);
    bind_PCAPUDPSource(m);
    bind_SC16ToBytes(m);
    bind_SC16ToComplex(m);
    bind_SC16ToIShort(m);
    bind_Signed8ToComplex(m);
    bind_tcp_sink(m);
    bind_udp_sink(m);
    bind_udp_source(m);
    // ) END BINDING_FUNCTION_CALLS
}