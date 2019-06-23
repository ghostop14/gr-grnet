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
#include "tcp_source_impl.h"
// 256 * 1024
// #define MAXQUEUESIZE 262144

namespace gr {
  namespace grnet {

    tcp_source::sptr
    tcp_source::make(size_t itemsize,size_t vecLen,int port)
    {
      return gnuradio::get_initial_sptr
        (new tcp_source_impl(itemsize, vecLen,port));
    }

    /*
     * The private constructor
     */
    tcp_source_impl::tcp_source_impl(size_t itemsize,size_t vecLen,int port)
      : gr::sync_block("tcp_source",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, itemsize*vecLen)),
    d_itemsize(itemsize), d_veclen(vecLen),d_port(port)//,    d_acceptor(d_io_service)
    {
    	d_block_size = d_itemsize * d_veclen;

    	stopThread = false;
    	threadRunning = false;
    	bConnected = false;
    	tcpsocket = NULL;

    	readThread = new boost::thread(boost::bind(&tcp_source_impl::readData, this));

    	// Set up listener
    	try {
		d_acceptor = new boost::asio::ip::tcp::acceptor(d_io_service,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),d_port));
    	}
    	catch(...) {
    		std::cerr << "[TCP Source] Error creating socket listener." << std::endl;
    		exit(1);
    	}

    	// Set up a different run thread for the io_service
		d_io_serv_thread = boost::thread(boost::bind(&boost::asio::io_service::run, &d_io_service));

    	connect(true);
    }

    void tcp_source_impl::connect(bool initialConnection) {
         std::cout << "TCP Source waiting for connection on port " << d_port << std::endl;
         if (!initialConnection) {
 			d_io_service.reset();
         }

         // d_acceptor.listen();

         // switched to match boost example code with a copy/temp pointer
         {
 			gr::thread::scoped_lock guard(d_socketmutex);
 			if (tcpsocket) {
 				delete tcpsocket;
 			}
 			tcpsocket = NULL; // new boost::asio::ip::tcp::socket(d_io_service);
 	        // This will block while waiting for a connection
 	        // d_acceptor.accept(*tcpsocket, d_endpoint);
 			bConnected = false;
         }

 		// Good full example:
 		// http://www.boost.org/doc/libs/1_36_0/doc/html/boost_asio/example/echo/async_tcp_echo_server.cpp
 		// Boost tutorial
 		// http://www.boost.org/doc/libs/1_63_0/doc/html/boost_asio/tutorial.html

 		boost::asio::ip::tcp::socket *tmpSocket = new boost::asio::ip::tcp::socket(d_io_service);
 		d_acceptor->async_accept(*tmpSocket,
 				boost::bind(&tcp_source_impl::accept_handler, this, tmpSocket, // This will make a ptr copy // boost::ref(tcpsocket),  // << pass by reference
 				          boost::asio::placeholders::error));

		d_io_service.run();
     }

     void tcp_source_impl::accept_handler(boost::asio::ip::tcp::socket * new_connection,
   	      const boost::system::error_code& error)
     {
       if (!error)
       {
           std::cout << "TCP Source Connection established." << std::endl;
         // Accept succeeded.
         gr::thread::scoped_lock guard(d_socketmutex);

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

     bool tcp_source_impl::stop() {
     	stopThread = true;

     	while (threadRunning) {
     		usleep(50);
     	}

     	delete readThread;
     	readThread = NULL;

         gr::thread::scoped_lock guard(d_socketmutex);

         if (tcpsocket) {
  			tcpsocket->close();
  			delete tcpsocket;
              tcpsocket = NULL;
          }

          d_io_service.reset();
          d_io_service.stop();
          d_io_serv_thread.join();

          if (d_acceptor) {
          	delete d_acceptor;
          	d_acceptor=NULL;
          }
          return true;
     }

	void tcp_source_impl::readData() {
    	threadRunning = true;
    	size_t bytesAvailable;

    	while (!stopThread) {
    		// If we're not connected just hang-out.
    		if (!bConnected) {
    			usleep(50);
    			continue;
    		}

        	bytesAvailable = netDataAvailable();

            if (bytesAvailable == 0) {
            	// check if we disconnected:
            	checkForDisconnect();
            }

        	size_t bytesRead;

        	// we could get here even if no data was received but there's still data in the queue.
        	// however read blocks so we want to make sure we have data before we call it.
        	if (bytesAvailable > 0) {
				// boost::array<char, bytesAvailable> buf;
            	// http://stackoverflow.com/questions/28929699/boostasio-read-n-bytes-from-socket-to-streambuf
        		{
					gr::thread::scoped_lock guard(d_socketmutex);
					bytesRead = boost::asio::read(*tcpsocket, read_buffer,boost::asio::transfer_exactly(bytesAvailable), ec);
					if (bytesRead != bytesAvailable) {
						std::cerr << "[TCP Source] ERROR Reading bytes.  " << bytesRead << " bytes read out of " << bytesAvailable << " bytes available" << std::endl;
					}
        		}

                if (ec) {
                	std::cout << "[TCP Source] TCP socket error: " << ec << std::endl;
                }

                if (bytesRead > 0) {
                	// commit the bytes to the stream
                    read_buffer.commit(bytesRead);

                    // Get the data and add it to our local queue.  We have to maintain a local queue
                    // in case we read more bytes than noutput_items is asking for.  In that case
                    // we'll only return noutput_items bytes
                    const char *readData = boost::asio::buffer_cast<const char*>(read_buffer.data());

                    queueData(readData,bytesRead);
                    // queueData(&buf[0],bytesRead);

                    // Tell the stream we've consumed the data.
                	read_buffer.consume(bytesRead);

                }
        	}

    		usleep(20);
    	}

    	threadRunning = false;
	}

	void tcp_source_impl::queueData(const char *data, long numBytes) {
    	// Lock mutex to add to the queue
		gr::thread::scoped_lock guard(d_mutex);

		for (long i=0;i<numBytes;i++) {
			localQueue.push(data[i]);
		}
	}

     /*
     * Our virtual destructor.
     */
    tcp_source_impl::~tcp_source_impl()
    {
    	stop();
    }

    size_t tcp_source_impl::dataAvailable() {
    	// Get amount of data available
        gr::thread::scoped_lock guard(d_mutex);

    	return localQueue.size();
    }

    size_t tcp_source_impl::netDataAvailable() {
    	// Get amount of data available

		if (tcpsocket) {

	    	boost::asio::socket_base::bytes_readable command(true);
			gr::thread::scoped_lock guard(d_socketmutex);

	    	tcpsocket->io_control(command);
	    	size_t bytes_readable = command.get();

			/*
			size_t bytes_readable = tcpsocket->available();

			if (bytes_readable > 0)
				std::cout << "Has bytes: " << bytes_readable << std::endl;
			*/
	    	return bytes_readable;
		}
		else {
			return 0;
		}
    }

    void tcp_source_impl::checkForDisconnect() {
    	// See https://sourceforge.net/p/asio/mailman/message/29070473/

    	int bytesRead;

    	char buff[1];
        bytesRead = tcpsocket->receive(boost::asio::buffer(buff),tcpsocket->message_peek, ec);
    	if ((boost::asio::error::eof == ec) ||(boost::asio::error::connection_reset == ec)) {
    		  std::cout << "Disconnect detected on port " << d_port << "." << std::endl;
    		  tcpsocket->close();
    		  delete tcpsocket;
    		  tcpsocket = NULL;

    		  // clear any queue items
    		  {
				  gr::thread::scoped_lock guard(d_mutex);
				  std::queue<char> empty;
				  std::swap(localQueue, empty );
    		  }

    	      connect(false);
  	    }
    	else {
    		if (ec) {
    			std::cout << "Socket error " << ec << " detected." << std::endl;
    		}
    	}
    }

    int
    tcp_source_impl::work_test(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
    	if (!bConnected)
    		return 0;

        size_t bytesAvailable = dataAvailable();

        if (bytesAvailable == 0) {
       		return 0;
        }

    	int localNumItems;
    	// let's figure out how much we have in relation to noutput_items
        localNumItems = bytesAvailable / d_block_size;

        // This takes care of if we have more data than is being requested
        if (localNumItems >= noutput_items) {
        	localNumItems = noutput_items;
        }

        // Now convert our block back to bytes
    	long bytesRequested = localNumItems * d_block_size;  // block size is sizeof(item) * vlen
        char *out = (char *) output_items[0];

        gr::thread::scoped_lock guard(d_mutex);

    	for (long i=0;i<bytesRequested;i++) {
    		out[i]=localQueue.front();
    		localQueue.pop();
    	}

    	// If we had less data than requested, it'll be reflected in the return value.
        return localNumItems;
    }

    int
    tcp_source_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
    	if (!bConnected)
    		return 0;

        size_t bytesAvailable = dataAvailable();

        if ((bytesAvailable == 0)) {
       		return 0;
        }

    	int localNumItems;
    	// let's figure out how much we have in relation to noutput_items
        localNumItems = bytesAvailable / d_block_size;

        // This takes care of if we have more data than is being requested
        if (localNumItems >= noutput_items) {
        	localNumItems = noutput_items;
        }

        if (localNumItems > 0) {
            // Now convert our block back to bytes
         	long bytesRequested = localNumItems * d_block_size;  // block size is sizeof(item) * vlen
             char *out = (char *) output_items[0];

             gr::thread::scoped_lock guard(d_mutex);

         	for (long i=0;i<bytesRequested;i++) {
         		out[i]=localQueue.front();
         		localQueue.pop();
         	}
       }

    	// If we had less data than requested, it'll be reflected in the return value.
        return localNumItems;
    }
  } /* namespace grnet */
} /* namespace gr */

