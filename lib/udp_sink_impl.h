/* -*- c++ -*- */
/* 
 * Copyright 2017 ghostop14.
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

#ifndef INCLUDED_GRNET_udp_sink_impl_H
#define INCLUDED_GRNET_udp_sink_impl_H

#include <grnet/udp_sink.h>
#include <boost/asio.hpp>
#include <boost/asio/ip/udp.hpp>
#include <queue>

#include "packet_headers.h"

namespace gr {
  namespace grnet {

    class udp_sink_impl : public udp_sink
    {
     protected:
    	int d_port;
        size_t d_itemsize;
        size_t d_veclen;
        size_t d_block_size;

        int d_header_type;
        int d_header_size;
        uint16_t d_payloadsize;
        uint64_t d_seq_num;
        bool b_send_eof;

        char tmpHeaderBuff[12];  // Largest header is 10 bytes so this should be big enough.

        std::queue<char> localQueue;
        unsigned char *localBuffer;


        boost::system::error_code ec;

        boost::asio::io_service d_io_service;
        boost::asio::ip::udp::endpoint d_endpoint;
        boost::asio::ip::udp::socket *udpsocket;

        boost::mutex d_mutex;

        virtual void buildHeader(); // returns header size.  Header is stored in tmpHeaderBuff

     public:
      udp_sink_impl(size_t itemsize,size_t vecLen,const std::string &host, int port,int headerType=HEADERTYPE_NONE,int payloadsize=1472,bool send_eof=true);
      ~udp_sink_impl();

      bool stop();

      // Where all the action really happens
      int work_test(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace grnet
} // namespace gr

#endif /* INCLUDED_GRNET_udp_sink_impl_H */

