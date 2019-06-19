# Copyright 2011 Free Software Foundation, Inc.
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

install(FILES
	grnet_tcp_sink.xml
	grnet_udp_sink.xml
	grnet_tcp_source.xml
	grnet_udp_source.xml
    grnet_PCAPUDPSource.xml
    grnet_ComplexToSigned8.xml
    grnet_Signed8ToComplex.xml
    grnet_ComplexToInt16Bytes.xml
    grnet_Int16BytesToComplex.xml
    grnet_FifoBuffer.xml
    grnet_SC16ToBytes.xml
    grnet_BytesToSC16.xml
    grnet_ByteComplexDecimator.xml DESTINATION share/gnuradio/grc/blocks
)
