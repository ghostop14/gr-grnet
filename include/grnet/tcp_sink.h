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

#ifndef INCLUDED_GRNET_TCP_SINK_H
#define INCLUDED_GRNET_TCP_SINK_H

#include <gnuradio/sync_block.h>
#include <grnet/api.h>

#define TCPSINKMODE_CLIENT 1
#define TCPSINKMODE_SERVER 2

namespace gr {
namespace grnet {

/*!
 * \brief This block provides a TCP Sink block that supports
 * both client and server modes.
 * \ingroup grnet
 *
 * \details
 * This block provides a TCP sink that supports both listening for
 * inbound connections (server mode) and connecting to other applications
 * (client mode) in order to send data from a GNU Radio flowgraph.
 * The block supports both IPv4 and IPv6 with appropriate code determined
 * by the address used.  In server mode, if a client disconnects, the
 * flowgraph will continue to execute.  If/when a new client connection
 * is established, data will then pick up with the current stream for
 * transmission to the new client.
 */
class GRNET_API tcp_sink : virtual public gr::sync_block {
public:
  typedef std::shared_ptr<tcp_sink> sptr;

  /*!
   * Build a tcp_sink block.
   */
  static sptr make(size_t itemsize, size_t vecLen, const std::string &host,
                   int port, int sinkMode);
};

} // namespace grnet
} // namespace gr

#endif /* INCLUDED_GRNET_TCP_SINK_H */
