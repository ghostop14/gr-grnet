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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "tcp_sink_impl.h"

namespace gr {
  namespace grnet {

    tcp_sink::sptr
    tcp_sink::make(size_t itemsize,size_t vecLen,const std::string &host, int port,bool noblock)
    {
    	std::cout << "TCP Sink connecting to " << host << ":" << port << std::endl;

      return gnuradio::get_initial_sptr
        (new tcp_sink_impl(itemsize, vecLen,host, port, noblock));
    }

    /*
     * The private constructor
     */
    tcp_sink_impl::tcp_sink_impl(size_t itemsize,size_t vecLen,const std::string &host, int port,bool noblock)
      : gr::sync_block("tcp_sink",
              gr::io_signature::make(1, 1, itemsize*vecLen),
              gr::io_signature::make(0, 0, 0)),
    d_itemsize(itemsize), d_veclen(vecLen)
    {
    	d_block_size = d_itemsize * d_veclen;

//    	std::cout << "TCP Sink connecting to " << host << ":" << port << std::endl;

        std::string s__port = (boost::format("%d") % port).str();
        std::string s__host = host; // host.empty() ? std::string("localhost") : host;
        boost::asio::ip::tcp::resolver resolver(d_io_service);
        boost::asio::ip::tcp::resolver::query query(s__host, s__port,
            boost::asio::ip::resolver_query_base::passive);
        d_endpoint = *resolver.resolve(query);

		tcpsocket = new boost::asio::ip::tcp::socket(d_io_service);
    	tcpsocket->connect(d_endpoint);

    	boost::asio::socket_base::keep_alive option(true);
    	tcpsocket->set_option(option);
    }

    /*
     * Our virtual destructor.
     */
    tcp_sink_impl::~tcp_sink_impl()
    {
    	stop();
    }

    bool tcp_sink_impl::stop() {
        if (tcpsocket) {
			tcpsocket->close();

            tcpsocket = NULL;

            d_io_service.reset();
            d_io_service.stop();
        }
        return true;
    }

    int
    tcp_sink_impl::work_test(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        gr::thread::scoped_lock guard(d_mutex);

        const char *in = (const char *) input_items[0];
    	unsigned int noi = noutput_items * d_block_size;

        // send it.
        // We could have done this async, however with guards and waits it has the same effect.
        // It just doesn't get detected till the next frame.
        // boost::asio::write(*tcpsocket, boost::asio::buffer(d_buf.get(), data_len),ec);
        boost::asio::write(*tcpsocket, boost::asio::buffer((const void *)in, noi),ec);

        return noutput_items;
    }

    int
    tcp_sink_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        gr::thread::scoped_lock guard(d_mutex);

        const char *in = (const char *) input_items[0];
    	unsigned int noi = noutput_items * d_block_size;

        // send it.
        // We could have done this async, however with guards and waits it has the same effect.
        // It just doesn't get detected till the next frame.
        // boost::asio::write(*tcpsocket, boost::asio::buffer(d_buf.get(), data_len),ec);
        boost::asio::write(*tcpsocket, boost::asio::buffer((const void *)in, noi),ec);

        return noutput_items;
    }
  } /* namespace grnet */
} /* namespace gr */

