/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(Int16BytesToComplex.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(a793abd7cb1cd2262afc492600d1d7b9)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <grnet/Int16BytesToComplex.h>
// pydoc.h is automatically generated in the build directory
#include <Int16BytesToComplex_pydoc.h>

void bind_Int16BytesToComplex(py::module& m)
{

    using Int16BytesToComplex    = ::gr::grnet::Int16BytesToComplex;


    py::class_<Int16BytesToComplex, gr::sync_decimator,
        std::shared_ptr<Int16BytesToComplex>>(m, "Int16BytesToComplex", D(Int16BytesToComplex))

        .def(py::init(&Int16BytesToComplex::make),
           D(Int16BytesToComplex,make)
        )
        



        ;




}








