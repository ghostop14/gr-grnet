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

#ifndef INCLUDED_GRNET_TCP_SINK_IMPL_H
#define INCLUDED_GRNET_TCP_SINK_IMPL_H

#include <grnet/tcp_sink.h>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

// using namespace boost::asio::ip::tcp;

namespace gr {
  namespace grnet {

    class GRNET_API tcp_sink_impl : public tcp_sink
    {
     protected:
        size_t d_itemsize;
        size_t d_veclen;
        size_t d_block_size;
        int d_sinkmode;
        bool is_ipv6;

        boost::system::error_code ec;

        boost::asio::io_service d_io_service;
        boost::asio::ip::tcp::endpoint d_endpoint;
        // std::set<boost::asio::ip::tcp::socket *> tcpsocket;
        boost::asio::ip::tcp::socket *tcpsocket=NULL;
        boost::asio::ip::tcp::acceptor *d_acceptor=NULL;

        boost::mutex d_mutex;

        std::string strHost;
        int d_port;

        bool bConnected;

        void checkForDisconnect();
        void connect(bool initialConnection);

     public:
      tcp_sink_impl(size_t itemsize,size_t vecLen,const std::string &host, int port,int sinkMode=TCPSINKMODE_CLIENT);
      ~tcp_sink_impl();

      bool stop();

      void accept_handler(boost::asio::ip::tcp::socket * new_connection,
    	      const boost::system::error_code& error);

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

#endif /* INCLUDED_GRNET_TCP_SINK_IMPL_H */

