/* -*- c++ -*- */

#define GRNET_API

%include "gnuradio.i"           // the common stuff

//load generated python docstrings
%include "grnet_swig_doc.i"

%{
#include "grnet/ComplexToSC16.h"
#include "grnet/SC16ToComplex.h"
#include "grnet/ByteComplexDecimator.h"
#include "grnet/BytesToSC16.h"
#include "grnet/SC16ToBytes.h"
#include "grnet/ComplexToInt16Bytes.h"
#include "grnet/Int16BytesToComplex.h"
#include "grnet/FifoBuffer.h"
#include "grnet/ComplexToSigned8.h"
#include "grnet/Signed8ToComplex.h"
#include "grnet/IShortToSC16.h"
#include "grnet/SC16ToIShort.h"
#include "grnet/PCAPUDPSource.h"
#include "grnet/tcp_sink.h"
#include "grnet/udp_source.h"
#include "grnet/udp_sink.h"
%}

%include "grnet/ComplexToSC16.h"
GR_SWIG_BLOCK_MAGIC2(grnet, ComplexToSC16);
%include "grnet/SC16ToComplex.h"
GR_SWIG_BLOCK_MAGIC2(grnet, SC16ToComplex);
%include "grnet/ByteComplexDecimator.h"
GR_SWIG_BLOCK_MAGIC2(grnet, ByteComplexDecimator);
%include "grnet/BytesToSC16.h"
GR_SWIG_BLOCK_MAGIC2(grnet, BytesToSC16);
%include "grnet/SC16ToBytes.h"
GR_SWIG_BLOCK_MAGIC2(grnet, SC16ToBytes);
%include "grnet/ComplexToInt16Bytes.h"
GR_SWIG_BLOCK_MAGIC2(grnet, ComplexToInt16Bytes);
%include "grnet/Int16BytesToComplex.h"
GR_SWIG_BLOCK_MAGIC2(grnet, Int16BytesToComplex);
%include "grnet/FifoBuffer.h"
GR_SWIG_BLOCK_MAGIC2(grnet, FifoBuffer);
%include "grnet/ComplexToSigned8.h"
GR_SWIG_BLOCK_MAGIC2(grnet, ComplexToSigned8);
%include "grnet/Signed8ToComplex.h"
GR_SWIG_BLOCK_MAGIC2(grnet, Signed8ToComplex);
%include "grnet/IShortToSC16.h"
GR_SWIG_BLOCK_MAGIC2(grnet, IShortToSC16);
%include "grnet/SC16ToIShort.h"
GR_SWIG_BLOCK_MAGIC2(grnet, SC16ToIShort);

%include "grnet/PCAPUDPSource.h"
GR_SWIG_BLOCK_MAGIC2(grnet, PCAPUDPSource);
%include "grnet/tcp_sink.h"
GR_SWIG_BLOCK_MAGIC2(grnet, tcp_sink);
%include "grnet/udp_source.h"
GR_SWIG_BLOCK_MAGIC2(grnet, udp_source);
%include "grnet/udp_sink.h"
GR_SWIG_BLOCK_MAGIC2(grnet, udp_sink);
