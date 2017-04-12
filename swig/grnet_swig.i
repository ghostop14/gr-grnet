/* -*- c++ -*- */

#define GRNET_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "grnet_swig_doc.i"

%{
#include "grnet/tcp_sink.h"
%}


%include "grnet/tcp_sink.h"
GR_SWIG_BLOCK_MAGIC2(grnet, tcp_sink);
