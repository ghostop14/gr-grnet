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
    tcp_sink::make(size_t itemsize,size_t vecLen,const std::string &host, int port,int sinkMode)
    {
    	std::cout << "TCP Sink connecting to " << host << ":" << port << std::endl;

      return gnuradio::get_initial_sptr
        (new tcp_sink_impl(itemsize, vecLen,host, port, sinkMode));
    }

    /*
     * The private constructor
     */
    tcp_sink_impl::tcp_sink_impl(size_t itemsize,size_t vecLen,const std::string &host, int port,int sinkMode)
      : gr::sync_block("tcp_sink",
              gr::io_signature::make(1, 1, itemsize*vecLen),
              gr::io_signature::make(0, 0, 0)),
    d_itemsize(itemsize), d_veclen(vecLen),d_port(port),strHost(host),d_acceptor(d_io_service),d_sinkmode(sinkMode)
    {
    	d_block_size = d_itemsize * d_veclen;

    	if (d_sinkmode==TCPSINKMODE_CLIENT) {
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
    	else {
    		connect(true);
    	}
    }

    void tcp_sink_impl::connect(bool initialConnection) {
        std::string s__port = (boost::format("%d") % d_port).str();
        std::string s__host = "0.0.0.0";
        boost::asio::ip::tcp::resolver resolver(d_io_service);
        boost::asio::ip::tcp::resolver::query query(s__host, s__port,
            boost::asio::ip::resolver_query_base::passive);
        d_endpoint = *resolver.resolve(query);

        std::cout << "TCP Source waiting for connection on " << s__host << ":" << d_port << std::endl;
        if (initialConnection) {
			d_acceptor.open(d_endpoint.protocol());
			d_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			d_acceptor.bind(d_endpoint);
        }

        d_acceptor.listen();

		tcpsocket = new boost::asio::ip::tcp::socket(d_io_service);
        // This will block while waiting for a connection
        d_acceptor.accept(*tcpsocket, d_endpoint);

    	boost::asio::socket_base::keep_alive option(true);
    	tcpsocket->set_option(option);
        std::cout << "TCP Source " << s__host << ":" << d_port << " connected." << std::endl;
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

        // read was blocking
        // checkForDisconnect();

        const char *in = (const char *) input_items[0];
    	unsigned int noi = noutput_items * d_block_size;
    	int bytesWritten;
    	int bytesRemaining = noi;
        // send it.
        // We could have done this async, however with guards and waits it has the same effect.
        // It just doesn't get detected till the next frame.
        // boost::asio::write(*tcpsocket, boost::asio::buffer(d_buf.get(), data_len),ec);
    	ec.clear();

    	while ((bytesRemaining > 0) && (!ec)) {
            bytesWritten = boost::asio::write(*tcpsocket, boost::asio::buffer((const void *)in, noi),ec);
            bytesRemaining -= bytesWritten;
    	}

        // writes happen a lot faster then reads.  To the point where it's overflowing the receiving buffer.
        // So this delay is to purposefully slow it down.
        //usleep(10);

        return noutput_items;
    }

    void tcp_sink_impl::checkForDisconnect() {
    	// See https://sourceforge.net/p/asio/mailman/message/29070473/

    	int bytesRead;

    	char buff[1];
        bytesRead = tcpsocket->receive(boost::asio::buffer(buff),tcpsocket->message_peek, ec);
    	if ((boost::asio::error::eof == ec) ||(boost::asio::error::connection_reset == ec)) {
    		  std::cout << "Disconnect detected on " << strHost << ":" << d_port << "." << std::endl;
    		  tcpsocket->close();
    		  delete tcpsocket;
    		  tcpsocket = NULL;

    	      exit(1);
  	    }
    	else {
    		if (ec) {
    			std::cout << "Socket error " << ec << " detected." << std::endl;
    		}
    	}
    }


    int
    tcp_sink_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        gr::thread::scoped_lock guard(d_mutex);

        // read was blocking
        // checkForDisconnect();

        const char *in = (const char *) input_items[0];
    	unsigned int noi = noutput_items * d_block_size;
    	int bytesWritten;
    	int bytesRemaining = noi;
        // send it.
        // We could have done this async, however with guards and waits it has the same effect.
        // It just doesn't get detected till the next frame.
        // boost::asio::write(*tcpsocket, boost::asio::buffer(d_buf.get(), data_len),ec);
    	ec.clear();

    	while ((bytesRemaining > 0) && (!ec)) {
            bytesWritten = boost::asio::write(*tcpsocket, boost::asio::buffer((const void *)in, noi),ec);
            bytesRemaining -= bytesWritten;
    	}

        // writes happen a lot faster then reads.  To the point where it's overflowing the receiving buffer.
        // So this delay is to purposefully slow it down.
        //usleep(100);

        return noutput_items;
    }
  } /* namespace grnet */
} /* namespace gr */

