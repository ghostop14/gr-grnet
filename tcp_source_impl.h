/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
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

#ifndef INCLUDED_GRNET_tcp_source_impl_H
#define INCLUDED_GRNET_tcp_source_impl_H

#include <grnet/tcp_source.h>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <queue>

namespace gr {
  namespace grnet {

    class tcp_source_impl : public tcp_source
    {
     private:
        size_t d_itemsize;
        size_t d_veclen;
        size_t d_block_size;

        int d_port;

        boost::system::error_code ec;

        boost::asio::io_service d_io_service;
        boost::asio::ip::tcp::endpoint d_endpoint;
        boost::asio::ip::tcp::socket *tcpsocket;
        boost::asio::ip::tcp::acceptor d_acceptor;

    	std::queue<char> localQueue;
        boost::asio::streambuf read_buffer;

        boost::mutex d_mutex;

        void connect(bool initialConnection);
        void checkForDisconnect();

     public:
      tcp_source_impl(size_t itemsize,size_t vecLen, int port);
      ~tcp_source_impl();

      bool stop();

      size_t dataAvailable();
      size_t netDataAvailable();

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

#endif /* INCLUDED_GRNET_tcp_source_impl_H */

