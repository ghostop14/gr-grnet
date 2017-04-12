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
    d_itemsize(itemsize), d_veclen(vecLen),
    d_buf(new uint8_t[BUF_SIZE]),
    d_writing(0)
    {
    	d_block_size = d_itemsize * d_veclen;

        std::string s__port = (boost::format("%d") % port).str();
        std::string s__host = host.empty() ? std::string("localhost") : host;
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
        gr::thread::scoped_lock guard(d_writing_mut);
        while (d_writing) {
          d_writing_cond.wait(guard);
        }

        if (tcpsocket) {
			tcpsocket->close();

            tcpsocket = NULL;

            d_io_service.reset();
            d_io_service.stop();
        }
        return true;
    }

    void
	tcp_sink_impl::do_write(const boost::system::error_code& error,size_t len)
    {
    	// Only used for async_write.  Not using this now but leaving it in.

        gr::thread::scoped_lock guard(d_writing_mut);
        --d_writing;

        if (error) {
        	std::cout << "Boost error code " << error << " sending data." << std::endl;
        }
      d_writing_cond.notify_one();
    }

    int
    tcp_sink_impl::work_test(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        const char *in = (const char *) input_items[0];
    	unsigned int noi = noutput_items * d_block_size;

    	// Set the amount of data we'll use.  noi or BUFF_SIZE
        size_t data_len = std::min(size_t(BUF_SIZE), (size_t)noi);
        data_len -= data_len % d_itemsize;

        // Copy our appropriate size to our local buffer
        memcpy(d_buf.get(), in, data_len);

        // send it.
        // We could have done this async, however with guards and waits it has the same effect.
        // It just doesn't get detected till the next frame.
        boost::asio::write(*tcpsocket, boost::asio::buffer(d_buf.get(), data_len),ec);

        /*
        if (ec) {
        	std::cout << "Boost write error: " << ec << std::endl;
        }
        else {
        	std::cout << "Wrote " << returnedBytes << "/" << data_len << " bytes." << std::endl;
        }
        */
        return data_len / d_veclen;
    }

    int
    tcp_sink_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        const char *in = (const char *) input_items[0];
    	unsigned int noi = noutput_items * d_block_size;

    	// Set the amount of data we'll use.  noi or BUFF_SIZE
        size_t data_len = std::min(size_t(BUF_SIZE), (size_t)noi);
        data_len -= data_len % d_itemsize;

        // Copy our appropriate size to our local buffer
        memcpy(d_buf.get(), in, data_len);

        // send it.
        // We could have done this async, however with guards and waits it has the same effect.
        // It just doesn't get detected till the next frame.
        boost::asio::write(*tcpsocket, boost::asio::buffer(d_buf.get(), data_len),ec);

        /*
        if (ec) {
        	std::cout << "Boost write error: " << ec << std::endl;
        }
        else {
        	std::cout << "Wrote " << returnedBytes << "/" << data_len << " bytes." << std::endl;
        }
        */
        return data_len / d_veclen;
    }
/*
    int
    tcp_sink_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        const char *in = (const char *) input_items[0];
    	size_t block_size = output_signature()->sizeof_stream_item (0);
    	unsigned int noi = d_itemsize * noutput_items;

        gr::thread::scoped_lock guard(d_writing_mut);
        while (d_writing) {
          d_writing_cond.wait(guard);
        }

        size_t data_len = std::min(size_t(BUF_SIZE), (size_t)noi);
        data_len -= data_len % d_itemsize;
        memcpy(d_buf.get(), in, data_len);

        std::cout << "Writing " << noi << " bytes" << std::endl;

        boost::asio::async_write(*tcpsocket, boost::asio::buffer(d_buf.get(), data_len),
            boost::bind(&tcp_sink_impl::do_write, this,
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred));
        d_writing++;

        return data_len / d_itemsize;
    }
    */
  } /* namespace grnet */
} /* namespace gr */

