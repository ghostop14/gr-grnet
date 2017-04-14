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
#include "udp_source_impl.h"

namespace gr {
  namespace grnet {

    udp_source::sptr
    udp_source::make(size_t itemsize,size_t vecLen,int port)
    {
      return gnuradio::get_initial_sptr
        (new udp_source_impl(itemsize, vecLen,port));
    }

    /*
     * The private constructor
     */
    udp_source_impl::udp_source_impl(size_t itemsize,size_t vecLen,int port)
      : gr::sync_block("udp_source",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, itemsize*vecLen)),
    d_itemsize(itemsize), d_veclen(vecLen)
    {
    	maxSize=256*1024;

    	d_block_size = d_itemsize * d_veclen;

    	/*
        std::string s__port = (boost::format("%d") % port).str();
        std::string s__host = "0.0.0.0";
        boost::asio::ip::udp::resolver resolver(d_io_service);
        boost::asio::ip::udp::resolver::query query(s__host, s__port,
            boost::asio::ip::resolver_query_base::passive);
        */
        d_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port);

		udpsocket = new boost::asio::ip::udp::socket(d_io_service,d_endpoint);

    }

    /*
     * Our virtual destructor.
     */
    udp_source_impl::~udp_source_impl()
    {
    	stop();
    }

    bool udp_source_impl::stop() {
        if (udpsocket) {
			udpsocket->close();

			udpsocket = NULL;

            d_io_service.reset();
            d_io_service.stop();
        }
        return true;
    }

    size_t udp_source_impl::dataAvailable() {
    	// Get amount of data available
    	boost::asio::socket_base::bytes_readable command(true);
    	udpsocket->io_control(command);
    	size_t bytes_readable = command.get();

    	return (bytes_readable+localQueue.size());
    }

    size_t udp_source_impl::netDataAvailable() {
    	// Get amount of data available
    	boost::asio::socket_base::bytes_readable command(true);
    	udpsocket->io_control(command);
    	size_t bytes_readable = command.get();

    	return bytes_readable;
    }


    int
    udp_source_impl::work_test(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        gr::thread::scoped_lock guard(d_mutex);

    	int bytesAvailable = netDataAvailable();

    	// quick exit if nothing to do
//        if ((bytesAvailable == 0) && (localQueue.size() == 0))
//        	return 0;

        char *out = (char *) output_items[0];
    	int bytesRead;
    	int returnedItems;
    	int localNumItems;
        int i;
    	unsigned int numRequested = noutput_items * d_block_size;

    	// we could get here even if no data was received but there's still data in the queue.
    	// however read blocks so we want to make sure we have data before we call it.
    	if (bytesAvailable > 0) {
            boost::asio::streambuf::mutable_buffers_type buf = read_buffer.prepare(numRequested);
        	// http://stackoverflow.com/questions/28929699/boostasio-read-n-bytes-from-socket-to-streambuf
            bytesRead = udpsocket->receive_from(buf,d_endpoint);
            read_buffer.commit(bytesRead);

            // Get the data and add it to our local queue.  We have to maintain a local queue
            // in case we read more bytes than noutput_items is asking for.  In that case
            // we'll only return noutput_items bytes
            const char *readData = boost::asio::buffer_cast<const char*>( read_buffer.data());

            for (i=0;i<bytesRead;i++) {
            	localQueue.push(readData[i]);
            }
    	}

    	read_buffer.consume(bytesRead);

    	// let's figure out how much we have in relation to noutput_items
        localNumItems = localQueue.size() / d_block_size;

        // This takes care of if we have more data than is being requested
        if (localNumItems >= noutput_items) {
        	localNumItems = noutput_items;
        }

        // Now convert our block back to bytes
    	unsigned int noi = localNumItems * d_block_size;  // block size is sizeof(item) * vlen

    	for (i=0;i<noi;i++) {
    		out[i]=localQueue.front();
    		localQueue.pop();
    	}

    	// If we had less data than requested, it'll be reflected in the return value.
        return localNumItems;
    }

    int
    udp_source_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        gr::thread::scoped_lock guard(d_mutex);

    	int bytesAvailable = netDataAvailable();

    	// quick exit if nothing to do
//        if ((bytesAvailable == 0) && (localQueue.size() == 0))
//        	return 0;

        char *out = (char *) output_items[0];
    	int bytesRead;
    	int returnedItems;
    	int localNumItems;
        int i;
    	unsigned int numRequested = noutput_items * d_block_size;

    	// we could get here even if no data was received but there's still data in the queue.
    	// however read blocks so we want to make sure we have data before we call it.
    	if (bytesAvailable > 0) {
            boost::asio::streambuf::mutable_buffers_type buf = read_buffer.prepare(numRequested);
        	// http://stackoverflow.com/questions/28929699/boostasio-read-n-bytes-from-socket-to-streambuf
            bytesRead = udpsocket->receive_from(buf,d_endpoint);
            read_buffer.commit(bytesRead);

            // Get the data and add it to our local queue.  We have to maintain a local queue
            // in case we read more bytes than noutput_items is asking for.  In that case
            // we'll only return noutput_items bytes
            const char *readData = boost::asio::buffer_cast<const char*>( read_buffer.data());

            for (i=0;i<bytesRead;i++) {
            	localQueue.push(readData[i]);
            }
        	read_buffer.consume(bytesRead);
    	}

    	// let's figure out how much we have in relation to noutput_items
        localNumItems = localQueue.size() / d_block_size;

        // This takes care of if we have more data than is being requested
        if (localNumItems >= noutput_items) {
        	localNumItems = noutput_items;
        }

        // Now convert our block back to bytes
    	unsigned int noi = localNumItems * d_block_size;  // block size is sizeof(item) * vlen

    	for (i=0;i<noi;i++) {
    		out[i]=localQueue.front();
    		localQueue.pop();
    	}

    	// If we had less data than requested, it'll be reflected in the return value.
        return localNumItems;
    }
  } /* namespace grnet */
} /* namespace gr */

