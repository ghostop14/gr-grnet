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
    d_itemsize(itemsize), d_veclen(vecLen),d_port(port),strHost(host),d_sinkmode(sinkMode) // ,d_acceptor(d_io_service)
    {
    	d_block_size = d_itemsize * d_veclen;

    	if (d_sinkmode==TCPSINKMODE_CLIENT) {
        	std::cout << "TCP Sink connecting to " << host << ":" << port << std::endl;

            std::string s__port = (boost::format("%d") % port).str();
            std::string s__host = host; // host.empty() ? std::string("localhost") : host;
            boost::asio::ip::tcp::resolver resolver(d_io_service);
            boost::asio::ip::tcp::resolver::query query(s__host, s__port,
                boost::asio::ip::resolver_query_base::passive);
            d_endpoint = *resolver.resolve(query);

    		tcpsocket = new boost::asio::ip::tcp::socket(d_io_service);

    		// this will block here.
        	tcpsocket->connect(d_endpoint);

        	bConnected = true;

        	boost::asio::socket_base::keep_alive option(true);
        	tcpsocket->set_option(option);
    	}
    	else {
    		connect(true);
    	}
    }

    void tcp_sink_impl::accept_handler(boost::asio::ip::tcp::socket * new_connection,
  	      const boost::system::error_code& error)
    {
      if (!error)
      {
          std::cout << "TCP Sink/Server Connection established." << std::endl;
        // Accept succeeded.
        tcpsocket = new_connection;

      	boost::asio::socket_base::keep_alive option(true);
      	tcpsocket->set_option(option);
      	bConnected = true;

      }
      else {
    	  std::cout << "Error code " << error << " accepting boost TCP session." << std::endl;

    	  // Boost made a copy so we have to clean up
    	  delete new_connection;

    	  // safety settings.
    	  bConnected = false;
    	  tcpsocket = NULL;
      }
    }

    void tcp_sink_impl::connect(bool initialConnection) {
    	/*
        std::string s__port = (boost::format("%d") % d_port).str();
        std::string s__host = "0.0.0.0";
        boost::asio::ip::tcp::resolver resolver(d_io_service);
        boost::asio::ip::tcp::resolver::query query(s__host, s__port,
            boost::asio::ip::resolver_query_base::passive);
        d_endpoint = *resolver.resolve(query);
		*/

        std::cout << "TCP Sink waiting for connection on port " << d_port << std::endl;
        if (initialConnection) {
        	/*
			d_acceptor.open(d_endpoint.protocol());
			d_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			d_acceptor.bind(d_endpoint);
			*/
	        d_acceptor = new boost::asio::ip::tcp::acceptor(d_io_service,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),d_port));
        }
        else {
			d_io_service.reset();
        }

        // d_acceptor.listen();

        // switched to match boost example code with a copy/temp pointer
        if (tcpsocket) {
        	delete tcpsocket;
        }
		tcpsocket = NULL; // new boost::asio::ip::tcp::socket(d_io_service);
        // This will block while waiting for a connection
        // d_acceptor.accept(*tcpsocket, d_endpoint);
		bConnected = false;


		// Good full example:
		// http://www.boost.org/doc/libs/1_36_0/doc/html/boost_asio/example/echo/async_tcp_echo_server.cpp
		// Boost tutorial
		// http://www.boost.org/doc/libs/1_63_0/doc/html/boost_asio/tutorial.html

		boost::asio::ip::tcp::socket *tmpSocket = new boost::asio::ip::tcp::socket(d_io_service);
		d_acceptor->async_accept(*tmpSocket,
				boost::bind(&tcp_sink_impl::accept_handler, this, tmpSocket, // This will make a ptr copy // boost::ref(tcpsocket),  // << pass by reference
				          boost::asio::placeholders::error));

		if (initialConnection) {
			d_io_service.run();
		}
		else {
			d_io_service.run();
		}
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
			delete tcpsocket;
            tcpsocket = NULL;
        }

        d_io_service.reset();
        d_io_service.stop();

        if (d_acceptor) {
        	delete d_acceptor;
        	d_acceptor=NULL;
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

    	if (!bConnected)
    		return noutput_items;

        const char *in = (const char *) input_items[0];
    	unsigned int noi = noutput_items * d_block_size;
    	int bytesWritten;
    	int bytesRemaining = noi;
        // send it.
        // We could have done this async, however with guards and waits it has the same effect.
        // It just doesn't get detected till the next frame.
        // boost::asio::write(*tcpsocket, boost::asio::buffer(d_buf.get(), data_len),ec);
    	ec.clear();

    	char *pBuff;
    	pBuff = (char *) input_items[0];

    	while ((bytesRemaining > 0) && (!ec)) {
            bytesWritten = boost::asio::write(*tcpsocket, boost::asio::buffer((const void *)pBuff, bytesRemaining),ec);
            bytesRemaining -= bytesWritten;
            pBuff += bytesWritten;

            if (ec == boost::asio::error::connection_reset || ec == boost::asio::error::broken_pipe) {
                // see http://stackoverflow.com/questions/3857272/boost-error-codes-reference for boost error codes


            	// Connection was reset
            	bConnected = false;
            	bytesRemaining = 0;

            	if (d_sinkmode==TCPSINKMODE_CLIENT) {
            		std::cout << "TCP Sink was disconnected. Stopping processing." << std::endl;


            		return WORK_DONE;
            	}
            	else {
                	std::cout << "TCP Sink [server mode] client disconnected." << std::endl;
                	connect(false);  // start waiting for another connection
            	}
            }

    	}

        // writes happen a lot faster then reads.  To the point where it's overflowing the receiving buffer.
        // So this delay is to purposefully slow it down.
        //usleep(100);

        return noutput_items;
    }
  } /* namespace grnet */
} /* namespace gr */

