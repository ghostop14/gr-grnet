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
/* BINDTOOL_HEADER_FILE(Signed8ToComplex.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(9bccfd60dae728b289eb1bba70c1f664)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <grnet/Signed8ToComplex.h>
// pydoc.h is automatically generated in the build directory
#include <Signed8ToComplex_pydoc.h>

void bind_Signed8ToComplex(py::module& m)
{

    using Signed8ToComplex    = ::gr::grnet::Signed8ToComplex;


    py::class_<Signed8ToComplex, gr::sync_decimator,
        std::shared_ptr<Signed8ToComplex>>(m, "Signed8ToComplex", D(Signed8ToComplex))

        .def(py::init(&Signed8ToComplex::make),
           D(Signed8ToComplex,make)
        )
        



        ;




}








