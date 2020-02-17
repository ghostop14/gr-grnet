/* -*- c++ -*- */
/*
 * Copyright 2017,2019,2020 ghostop14.
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

#ifndef INCLUDED_GRNET_udp_source_impl_H
#define INCLUDED_GRNET_udp_source_impl_H

#include <boost/asio.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/circular_buffer.hpp>
#include <grnet/udp_source.h>

#include "packet_headers.h"

namespace gr {
namespace grnet {

class GRNET_API udp_source_impl : public udp_source {
protected:
  size_t d_itemsize;
  size_t d_veclen;
  size_t d_block_size;

  bool d_notifyMissed;
  bool d_sourceZeros;
  int d_partialFrameCounter;

  bool is_ipv6;

  int d_port;
  int d_header_type;
  int d_header_size;
  uint16_t d_payloadsize;
  int d_precompDataSize;
  int d_precompDataOverItemSize;
  long d_udp_recv_buf_size;

  uint64_t d_seq_num;
  unsigned char *localBuffer;

  boost::system::error_code ec;

  boost::asio::io_service d_io_service;
  boost::asio::ip::udp::endpoint d_endpoint;
  boost::asio::ip::udp::socket *d_udpsocket;

  boost::asio::streambuf d_read_buffer;

  // A queue is required because we have 2 different timing
  // domains: The network packets and the GR work()/scheduler
  boost::circular_buffer<unsigned char> *d_localqueue;

  uint64_t get_header_seqnum();

public:
  udp_source_impl(size_t itemsize, size_t vecLen, int port, int headerType,
                  int payloadsize, long udp_recv_buf_size, bool notifyMissed,
                  bool sourceZeros, bool ipv6);
  ~udp_source_impl();

  bool stop();

  size_t data_available();
  inline size_t netdata_available();

  // Where all the action really happens
  int work_test(int noutput_items, gr_vector_const_void_star &input_items,
                gr_vector_void_star &output_items);
  int work(int noutput_items, gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
};

} // namespace grnet
} // namespace gr

#endif /* INCLUDED_GRNET_udp_source_impl_H */
