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
#include "udp_sink_impl.h"
#include <zlib.h>

namespace gr {
  namespace grnet {

    udp_sink::sptr
    udp_sink::make(size_t itemsize,size_t vecLen,const std::string &host, int port,int headerType,int payloadsize)
    {
      return gnuradio::get_initial_sptr
        (new udp_sink_impl(itemsize, vecLen,host, port,headerType,payloadsize));
    }

    /*
     * The private constructor
     */
    udp_sink_impl::udp_sink_impl(size_t itemsize,size_t vecLen,const std::string &host, int port,int headerType,int payloadsize)
      : gr::sync_block("udp_sink",
              gr::io_signature::make(1, 1, itemsize*vecLen),
              gr::io_signature::make(0, 0, 0)),
    d_itemsize(itemsize), d_veclen(vecLen),d_header_type(headerType),d_seq_num(0),d_header_size(0),d_payloadsize(payloadsize)
    {
    	if (d_payloadsize<8) {
  		  std::cout << "Error: payload size is too small.  Must be at least 8 bytes" << std::endl;
  	      exit(1);
    	}

    	d_block_size = d_itemsize * d_veclen;

    	switch (d_header_type) {
    	case HEADERTYPE_NONE:
    		d_header_size = 0;
    	break;

    	case HEADERTYPE_SEQNUM:
    		d_header_size = 8;
    	break;
    	case HEADERTYPE_SEQPLUSSIZE:
    	case HEADERTYPE_SEQSIZECRC:
    		d_header_size = 12;
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
        if (udpsocket) {
        	udpsocket->close();

        	udpsocket = NULL;

            d_io_service.reset();
            d_io_service.stop();
        }
        return true;
    }

    int
    udp_sink_impl::work_test(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        gr::thread::scoped_lock guard(d_mutex);

        int maxDataSize=d_payloadsize;
        switch (d_header_type) {
        	case HEADERTYPE_NONE:
        		maxDataSize=d_payloadsize;
        	break;
        	case HEADERTYPE_SEQNUM:
        		maxDataSize = d_payloadsize - 8;
        	break;

        	case HEADERTYPE_SEQPLUSSIZE:
        		maxDataSize = d_payloadsize - 12;
        	break;

        	case HEADERTYPE_SEQSIZECRC:
        		maxDataSize = d_payloadsize - 12 - sizeof(unsigned long);
        	break;
        }

        const char *in = (const char *) input_items[0];
    	unsigned int noi = noutput_items * d_block_size;

    	// Calc our packet break-up
    	float fNumPackets = (float)noi / (float)maxDataSize;
    	int numPackets = noi / maxDataSize;
    	if (fNumPackets > (float)numPackets)
    		numPackets = numPackets + 1;

    	// deal with a last packet < maxDataSize
    	int lastPacketSize = noi - (noi / maxDataSize) * maxDataSize;

    	std::vector<boost::asio::const_buffer> transmitbuffer;
    	int curPtr=0;

    	for (int curPacket=0;curPacket < numPackets; curPacket++) {
    		// Clear the next transmit buffer
    		transmitbuffer.clear();

    		// build our next header if we need it
            if (d_header_type != HEADERTYPE_NONE) {
            	if (d_seq_num == 0xFFFFFFFF)
            		d_seq_num = 0;

            	d_seq_num++;
            	// want to send the header.
            	tmpHeaderBuff[0]=tmpHeaderBuff[1]=tmpHeaderBuff[2]=tmpHeaderBuff[3]=0xFF;

                memcpy((void *)&tmpHeaderBuff[4], (void *)&d_seq_num, sizeof(d_seq_num));

                if ((d_header_type == HEADERTYPE_SEQPLUSSIZE)||(d_header_type == HEADERTYPE_SEQSIZECRC)) {
                    if (curPacket < (numPackets-1))
                        memcpy((void *)&tmpHeaderBuff[8], (void *)&maxDataSize, sizeof(maxDataSize));
                    else
                        memcpy((void *)&tmpHeaderBuff[8], (void *)&lastPacketSize, sizeof(lastPacketSize));
                }

                transmitbuffer.push_back(boost::asio::buffer((const void *)tmpHeaderBuff, d_header_size));

                // udpsocket->send_to(boost::asio::buffer((const void *)tmpHeaderBuff, d_header_size),d_endpoint);
            }

            // Grab our next data chunk
            if (curPacket < (numPackets-1)) {
            	transmitbuffer.push_back(boost::asio::buffer((const void *)&in[curPtr], maxDataSize));
            }
            else {
            	transmitbuffer.push_back(boost::asio::buffer((const void *)&in[curPtr], lastPacketSize));
            }
            // Actual transmit is now further down
            // udpsocket->send_to(boost::asio::buffer((const void *)in, noi),d_endpoint);

            if (d_header_type == HEADERTYPE_SEQSIZECRC) {
            	unsigned long  crc = crc32(0L, Z_NULL, 0);
                if (curPacket < (numPackets-1))
                	crc = crc32(crc, (const unsigned char*)&in[curPtr], maxDataSize);
                else
                	crc = crc32(crc, (const unsigned char*)&in[curPtr], lastPacketSize);

                memcpy((void *)tmpHeaderBuff, (void *)&crc, sizeof(crc));
                transmitbuffer.push_back(boost::asio::buffer((const void *)tmpHeaderBuff, sizeof(crc)));
                // udpsocket->send_to(boost::asio::buffer((const void *)tmpHeaderBuff, sizeof(crc)),d_endpoint);
            }

            udpsocket->send_to(transmitbuffer,d_endpoint);

            curPtr = curPtr + maxDataSize;
    	}

        return noutput_items;
    }

    int
    udp_sink_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
        gr::thread::scoped_lock guard(d_mutex);

        int maxDataSize=d_payloadsize;
        switch (d_header_type) {
        	case HEADERTYPE_NONE:
        		maxDataSize=d_payloadsize;
        	break;
        	case HEADERTYPE_SEQNUM:
        		maxDataSize = d_payloadsize - 8;
        	break;

        	case HEADERTYPE_SEQPLUSSIZE:
        		maxDataSize = d_payloadsize - 12;
        	break;

        	case HEADERTYPE_SEQSIZECRC:
        		maxDataSize = d_payloadsize - 12 - sizeof(unsigned long);
        	break;
        }

        const char *in = (const char *) input_items[0];
    	unsigned int noi = noutput_items * d_block_size;

    	// Calc our packet break-up
    	float fNumPackets = (float)noi / (float)maxDataSize;
    	int numPackets = noi / maxDataSize;
    	if (fNumPackets > (float)numPackets)
    		numPackets = numPackets + 1;

    	// deal with a last packet < maxDataSize
    	int lastPacketSize = noi - (noi / maxDataSize) * maxDataSize;

    	std::vector<boost::asio::const_buffer> transmitbuffer;
    	int curPtr=0;

    	for (int curPacket=0;curPacket < numPackets; curPacket++) {
    		// Clear the next transmit buffer
    		transmitbuffer.clear();

    		// build our next header if we need it
            if (d_header_type != HEADERTYPE_NONE) {
            	if (d_seq_num == 0xFFFFFFFF)
            		d_seq_num = 0;

            	d_seq_num++;
            	// want to send the header.
            	tmpHeaderBuff[0]=tmpHeaderBuff[1]=tmpHeaderBuff[2]=tmpHeaderBuff[3]=0xFF;

                memcpy((void *)&tmpHeaderBuff[4], (void *)&d_seq_num, sizeof(d_seq_num));

                if ((d_header_type == HEADERTYPE_SEQPLUSSIZE)||(d_header_type == HEADERTYPE_SEQSIZECRC)) {
                    if (curPacket < (numPackets-1))
                        memcpy((void *)&tmpHeaderBuff[8], (void *)&maxDataSize, sizeof(maxDataSize));
                    else
                        memcpy((void *)&tmpHeaderBuff[8], (void *)&lastPacketSize, sizeof(lastPacketSize));
                }

                transmitbuffer.push_back(boost::asio::buffer((const void *)tmpHeaderBuff, d_header_size));

                // udpsocket->send_to(boost::asio::buffer((const void *)tmpHeaderBuff, d_header_size),d_endpoint);
            }

            // Grab our next data chunk
            if (curPacket < (numPackets-1)) {
            	transmitbuffer.push_back(boost::asio::buffer((const void *)&in[curPtr], maxDataSize));
            }
            else {
            	transmitbuffer.push_back(boost::asio::buffer((const void *)&in[curPtr], lastPacketSize));
            }
            // Actual transmit is now further down
            // udpsocket->send_to(boost::asio::buffer((const void *)in, noi),d_endpoint);

            if (d_header_type == HEADERTYPE_SEQSIZECRC) {
            	unsigned long  crc = crc32(0L, Z_NULL, 0);
                if (curPacket < (numPackets-1))
                	crc = crc32(crc, (const unsigned char*)&in[curPtr], maxDataSize);
                else
                	crc = crc32(crc, (const unsigned char*)&in[curPtr], lastPacketSize);

                memcpy((void *)tmpHeaderBuff, (void *)&crc, sizeof(crc));
                transmitbuffer.push_back(boost::asio::buffer((const void *)tmpHeaderBuff, sizeof(crc)));
                // udpsocket->send_to(boost::asio::buffer((const void *)tmpHeaderBuff, sizeof(crc)),d_endpoint);
            }

            udpsocket->send_to(transmitbuffer,d_endpoint);

            curPtr = curPtr + maxDataSize;
    	}

        return noutput_items;
    }
  } /* namespace grnet */
} /* namespace gr */

