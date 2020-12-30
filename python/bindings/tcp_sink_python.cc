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
/* BINDTOOL_HEADER_FILE(tcp_sink.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(237eba72fe0c5f765bc782e08a65a2c0)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <grnet/tcp_sink.h>
// pydoc.h is automatically generated in the build directory
#include <tcp_sink_pydoc.h>

void bind_tcp_sink(py::module& m)
{

    using tcp_sink    = ::gr::grnet::tcp_sink;


    py::class_<tcp_sink, gr::sync_block, gr::block, gr::basic_block,
        std::shared_ptr<tcp_sink>>(m, "tcp_sink", D(tcp_sink))

        .def(py::init(&tcp_sink::make),
           py::arg("itemsize"),
           py::arg("vecLen"),
           py::arg("host"),
           py::arg("port"),
           py::arg("sinkMode"),
           D(tcp_sink,make)
        )
        



        ;




}








