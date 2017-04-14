/* -*- c++ -*- */

#define GRNET_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "grnet_swig_doc.i"

%{
#include "grnet/tcp_sink.h"
#include "grnet/tcp_source.h"
#include "grnet/udp_sink.h"
#include "grnet/udp_source.h"
%}


%include "grnet/tcp_sink.h"
GR_SWIG_BLOCK_MAGIC2(grnet, tcp_sink);

%include "grnet/tcp_source.h"
GR_SWIG_BLOCK_MAGIC2(grnet, tcp_source);

%include "grnet/udp_sink.h"
GR_SWIG_BLOCK_MAGIC2(grnet, udp_sink);

%include "grnet/udp_source.h"
GR_SWIG_BLOCK_MAGIC2(grnet, udp_source);
