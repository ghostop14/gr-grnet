/* -*- c++ -*- */
/*
 * Copyright 2019 ghostop14.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_GRNET_UDP_SINK_H
#define INCLUDED_GRNET_UDP_SINK_H

#include <gnuradio/sync_block.h>
#include <grnet/api.h>

namespace gr {
namespace grnet {

/*!
 * \brief This block provides a UDP Sink block that can send
 * data over UDP streams to a specified host.
 * \ingroup grnet
 *
 * \details
 * This block provides a UDP sink that supports sending data over
 * a UDP stream to external applications.  A number of header formats
 * are supported including None (raw stream), and other header formats
 * that allow for sequence numbers to be tracked.  This feature allows
 * the recipient to be aware of any frames dropped in transit or by
 * its receiving stack.  The UDP packet size can also be adjusted
 * to support jumbo frames.  For most networks, 1472 is the correct
 * UDP data packet size that optimizes network transmission.  Adjusting
 * this value without a full understanding of the network implications
 * can create additional network fragmentation and inefficient packet
 * usage so should be avoided.  For networks and endpoints supporting
 * jumbo frames of 9000, 8972 would be the appropriate size
 * (9000 - 28 header bytes).  If send NULL packet as EOF is set, when
 * the flowgraph terminates, an empty UDP packet is sent.  This can
 * be used on the receiving side to be aware that no more data may
 * be received from the sending application.  When pairing with the
 * GNU Radio UDP source block, this isn't necessary and the source
 * block can simply be set to source zero's when an incoming network
 * data stream is not present in order to continue returning data
 * from the work function.  This block also supports IPv4 and IPv6
 * addresses and is automatically determined from the address
 * provided.
 */
class GRNET_API udp_sink : virtual public gr::sync_block {
public:
  typedef boost::shared_ptr<udp_sink> sptr;

  /*!
   * Build a udp_sink block.
   */
  static sptr make(size_t itemsize, size_t vecLen, const std::string &host,
                   int port, int headerType, int payloadsize, bool send_eof);
};

} // namespace grnet
} // namespace gr

#endif /* INCLUDED_GRNET_UDP_SINK_H */
