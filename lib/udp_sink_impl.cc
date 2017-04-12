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
#include "udp_sink_impl.h"
#include <zlib.h>

namespace gr {
  namespace grnet {

    udp_sink::sptr
    udp_sink::make(size_t itemsize,size_t vecLen,const std::string &host, int port,int headerType)
    {
      return gnuradio::get_initial_sptr
        (new udp_sink_impl(itemsize, vecLen,host, port,headerType));
    }

    /*
     * The private constructor
     */
    udp_sink_impl::udp_sink_impl(size_t itemsize,size_t vecLen,const std::string &host, int port,int headerType)
      : gr::sync_block("udp_sink",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, itemsize*vecLen)),
    d_itemsize(itemsize), d_veclen(vecLen),d_header_type(headerType),d_seq_num(0),d_header_size(0),
    d_buf(new uint8_t[BUF_SIZE]),
    d_writing(0)
    {
    	d_block_size = d_itemsize * d_veclen;

    	switch (d_header_type) {
    	case HEADERTYPE_NONE:
    		d_header_size = 0;
    	break;

    	case HEADERTYPE_SEQNUM:
    		d_header_size = 4;
    	break;
    	case HEADERTYPE_SEQPLUSSIZE:
    	case HEADERTYPE_SEQSIZECRC:
    		d_header_size = 8;
    	break;
    	}

        std::string s__port = (boost::format("%d") % port).str();
        std::string s__host = host.empty() ? std::string("localhost") : host;
        boost::asio::ip::udp::resolver resolver(d_io_service);
        boost::asio::ip::udp::resolver::query query(s__host, s__port,
            boost::asio::ip::resolver_query_base::passive);
        d_endpoint = *resolver.resolve(query);

		udpsocket = new boost::asio::ip::udp::socket(d_io_service);
    	udpsocket->connect(d_endpoint);
    }

    /*
     * Our virtual destructor.
     */
    udp_sink_impl::~udp_sink_impl()
    {
    	stop();
    }

    bool udp_sink_impl::stop() {
        gr::thread::scoped_lock guard(d_writing_mut);
        /*
        while (d_writing) {
          d_writing_cond.wait(guard);
        }
		*/
        if (udpsocket) {
        	udpsocket->close();

        	udpsocket = NULL;

            d_io_service.reset();
            d_io_service.stop();
        }
        return true;
    }

    void
	udp_sink_impl::do_write(const boost::system::error_code& error,size_t len)
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
    udp_sink_impl::work_test(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        const char *in = (const char *) input_items[0];
    	unsigned int noi = noutput_items * d_block_size;

    	// Set the amount of data we'll use.  noi or BUFF_SIZE
        size_t data_len = std::min(size_t(BUF_SIZE), (size_t)noi);
        data_len -= data_len % d_itemsize;

        if (d_header_type != HEADERTYPE_NONE) {
        	if (d_seq_num == 0xFFFFFFFF)
        		d_seq_num = 0;

        	d_seq_num++;
        	// want to send the header.
            memcpy((void *)tmpHeaderBuff, (void *)&d_seq_num, sizeof(d_seq_num));

            if (d_header_type == HEADERTYPE_SEQPLUSSIZE) {
                memcpy((void *)&tmpHeaderBuff[3], (void *)&noi, sizeof(noi));
            }
            udpsocket->send_to(boost::asio::buffer((const void *)tmpHeaderBuff, d_header_size),d_endpoint);
        }

        // Copy our appropriate size to our local buffer
        memcpy(d_buf.get(), in, data_len);

        // send it.
        // We could have done this async, however with guards and waits it has the same effect.
        // It just doesn't get detected till the next frame.
        udpsocket->send_to(boost::asio::buffer(d_buf.get(), data_len),d_endpoint);

        if (d_header_type == HEADERTYPE_SEQSIZECRC) {
        	unsigned long  crc = crc32(0L, Z_NULL, 0);
        	crc = crc32(crc, (const unsigned char*)d_buf.get(), data_len);
            memcpy((void *)tmpHeaderBuff, (void *)&crc, sizeof(crc));
            udpsocket->send_to(boost::asio::buffer((const void *)tmpHeaderBuff, sizeof(crc)),d_endpoint);
        }
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
    udp_sink_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        const char *in = (const char *) input_items[0];
    	unsigned int noi = noutput_items * d_block_size;

    	// Set the amount of data we'll use.  noi or BUFF_SIZE
        size_t data_len = std::min(size_t(BUF_SIZE), (size_t)noi);
        data_len -= data_len % d_itemsize;

        if (d_header_type != HEADERTYPE_NONE) {
        	if (d_seq_num == 0xFFFFFFFF)
        		d_seq_num = 0;

        	d_seq_num++;
        	// want to send the header.
            memcpy((void *)tmpHeaderBuff, (void *)&d_seq_num, sizeof(d_seq_num));

            if (d_header_type == HEADERTYPE_SEQPLUSSIZE) {
                memcpy((void *)&tmpHeaderBuff[3], (void *)&noi, sizeof(noi));
            }
            udpsocket->send_to(boost::asio::buffer((const void *)tmpHeaderBuff, d_header_size),d_endpoint);
        }
        // Copy our appropriate size to our local buffer
        memcpy(d_buf.get(), in, data_len);

        // send it.
        // We could have done this async, however with guards and waits it has the same effect.
        // It just doesn't get detected till the next frame.
        udpsocket->send_to(boost::asio::buffer(d_buf.get(), data_len),d_endpoint);

        if (d_header_type == HEADERTYPE_SEQSIZECRC) {
        	unsigned long  crc = crc32(0L, Z_NULL, 0);
        	crc = crc32(crc, (const unsigned char*)d_buf.get(), data_len);
            memcpy((void *)tmpHeaderBuff, (void *)&crc, sizeof(crc));
            udpsocket->send_to(boost::asio::buffer((const void *)tmpHeaderBuff, sizeof(crc)),d_endpoint);
        }
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
  } /* namespace grnet */
} /* namespace gr */

