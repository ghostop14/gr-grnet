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

#ifndef INCLUDED_GRNET_UDP_SOURCE_H
#define INCLUDED_GRNET_UDP_SOURCE_H

#include <gnuradio/sync_block.h>
#include <grnet/api.h>

namespace gr {
namespace grnet {

/*!
 * \brief This block provides a UDP source block that starts a
 * listener on the specified port and waits for inbound UDP packets.
 * \ingroup grnet
 *
 * \details
 * This block provides a UDP source that supports receiving data over
 * a UDP stream from external applications.  A number of header formats
 * are supported including None (raw stream), and other header formats
 * that allow for sequence numbers to be tracked.  This feature allows
 * the flowgraph to be aware of any frames dropped in transit or by
 * its receiving stack.  However, this needs to be appropriately
 * paired with the sending application (it needs to send the same
 * header).  The UDP packet size can also be adjusted
 * to support jumbo frames.  For most networks, 1472 is the correct
 * UDP data packet size that optimizes network transmission.  Adjusting
 * this value without a full understanding of the network implications
 * can create additional network fragmentation and inefficient packet
 * usage so should be avoided.  For networks and endpoints supporting
 * jumbo frames of 9000, 8972 would be the appropriate size
 * (9000 - 28 header bytes).  This block does support IPv4 only or
 * dual stack IPv4/IPv6 listening as an endpoint with an enable
 * IPv6 option that can be set on the block properties page.  It can
 * also be set to source zeros (no signal) in the event no data
 * is being received.
 */
class GRNET_API udp_source : virtual public gr::sync_block {
public:
  typedef boost::shared_ptr<udp_source> sptr;

  /*!
   * Build a udp_source block.
   */
  static sptr make(size_t itemsize, size_t vecLen, int port, int headerType,
                   int payloadsize, bool notifyMissed,
                   bool sourceZeros, bool ipv6);
};

} // namespace grnet
} // namespace gr

#endif /* INCLUDED_GRNET_UDP_SOURCE_H */
