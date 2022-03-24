/* -*- c++ -*- */
/*
 * Copyright 2017,2019,2020 ghostop14.
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

#include "udp_sink_impl.h"
#include <boost/array.hpp>
#include <boost/format.hpp>
#include <gnuradio/io_signature.h>

namespace gr {
namespace grnet {

udp_sink::sptr udp_sink::make(size_t itemsize, size_t vecLen,
                              const std::string &host, int port, int headerType,
                              int payloadsize, bool send_eof) {
  return gnuradio::get_initial_sptr(new udp_sink_impl(
      itemsize, vecLen, host, port, headerType, payloadsize, send_eof));
}

/*
 * The private constructor
 */
udp_sink_impl::udp_sink_impl(size_t itemsize, size_t vecLen,
                             const std::string &host, int port, int headerType,
                             int payloadsize, bool send_eof)
    : gr::sync_block("udp_sink",
                     gr::io_signature::make(1, 1, itemsize * vecLen),
                     gr::io_signature::make(0, 0, 0)),
      d_itemsize(itemsize), d_veclen(vecLen), d_header_type(headerType),
      d_seq_num(0), d_header_size(0), d_payloadsize(payloadsize),
      b_send_eof(send_eof) {
  // Lets set up the max payload size for the UDP packet based on the requested
  // payload size. Some important notes:  For a standard IP/UDP packet, say
  // crossing the Internet with a standard MTU, 1472 is the max UDP payload
  // size.  Larger values can be sent, however the IP stack will fragment the
  // packet.  This can cause additional network overhead as the packet gets
  // reassembled. Now for local nets that support jumbo frames, the max payload
  // size is 8972 (9000-the UDP 28-byte header) Same rules apply with
  // fragmentation.

  d_port = port;

  d_header_size = 0;

  switch (d_header_type) {
  case HEADERTYPE_SEQNUM:
    d_header_size = sizeof(HeaderSeqNum);
    break;

  case HEADERTYPE_SEQPLUSSIZE:
    d_header_size = sizeof(HeaderSeqPlusSize);
    break;

  case HEADERTYPE_CHDR:
    d_header_size = sizeof(CHDR);
    break;

  case HEADERTYPE_NONE:
    d_header_size = 0;
    break;

  default:
    GR_LOG_ERROR(d_logger, "Unknown header type.");
    exit(1);
    break;
  }

  if (d_payloadsize < 8) {
    GR_LOG_ERROR(d_logger,
                 "Payload size is too small.  Must be at "
                 "least 8 bytes once header/trailer adjustments are made.");
    exit(1);
  }

  d_seq_num = 0;

  d_block_size = d_itemsize * d_veclen;

  d_precomp_datasize = d_payloadsize - d_header_size;
  d_precomp_data_overitemsize = d_precomp_datasize / d_itemsize;

  d_localbuffer = new char[d_payloadsize];

  long max_circ_buffer;

  // Let's keep it from getting too big
  if (d_payloadsize < 2000) {
      max_circ_buffer = d_payloadsize * 4000;
  } else {
      if (d_payloadsize < 5000)
          max_circ_buffer = d_payloadsize * 2000;
      else
          max_circ_buffer = d_payloadsize * 1500;
  }

  d_localqueue = new boost::circular_buffer<char>(max_circ_buffer);

  d_udpsocket = new boost::asio::ip::udp::socket(d_io_service);

  std::string s_port = (boost::format("%d") % port).str();
  std::string s_host = host.empty() ? std::string("localhost") : host;
  boost::asio::ip::udp::resolver resolver(d_io_service);
  boost::asio::ip::udp::resolver::query query(
      s_host, s_port, boost::asio::ip::resolver_query_base::passive);

  boost::system::error_code err;
  d_endpoint = *resolver.resolve(query, err);

  if (err) {
    throw std::runtime_error(
        std::string("[UDP Sink] Unable to resolve host/IP: ") + err.message());
  }

  if (host.find(":") != std::string::npos)
    is_ipv6 = true;
  else {
    // This block supports a check that a name rather than an IP is provided.
    // the endpoint is then checked after the resolver is done.
    if (d_endpoint.address().is_v6())
      is_ipv6 = true;
    else
      is_ipv6 = false;
  }

  if (is_ipv6) {
    d_udpsocket->open(boost::asio::ip::udp::v6());
  } else {
    d_udpsocket->open(boost::asio::ip::udp::v4());
  }

  int out_multiple = (d_payloadsize - d_header_size) / d_block_size;

  if (out_multiple == 1)
    out_multiple =
        2; // Ensure we get pairs, for instance complex -> ichar pairs

  gr::block::set_output_multiple(out_multiple);
}

/*
 * Our virtual destructor.
 */
udp_sink_impl::~udp_sink_impl() { stop(); }

bool udp_sink_impl::stop() {
  if (d_udpsocket) {
    gr::thread::scoped_lock guard(d_setlock);

    if (b_send_eof) {
      // Send a few zero-length packets to signal receiver we are done
      boost::array<char, 0> send_buf;
      for (int i = 0; i < 3; i++)
        d_udpsocket->send_to(boost::asio::buffer(send_buf), d_endpoint);
    }

    d_udpsocket->close();
    d_udpsocket = NULL;

    d_io_service.reset();
    d_io_service.stop();
  }

  if (d_localbuffer) {
    delete[] d_localbuffer;
    d_localbuffer = NULL;
  }

  if (d_localqueue) {
      delete d_localqueue;
      d_localqueue = NULL;
  }

  return true;
}

void udp_sink_impl::build_header() {
  switch (d_header_type) {
  case HEADERTYPE_SEQNUM: {
    d_seq_num++;
    HeaderSeqNum seqHeader;
    seqHeader.seqnum = d_seq_num;
    memcpy((void *)d_tmpheaderbuff, (void *)&seqHeader, d_header_size);
  } break;

  case HEADERTYPE_SEQPLUSSIZE: {
    d_seq_num++;
    HeaderSeqPlusSize seqHeaderPlusSize;
    seqHeaderPlusSize.seqnum = d_seq_num;
    seqHeaderPlusSize.length = d_payloadsize;
    memcpy((void *)d_tmpheaderbuff, (void *)&seqHeaderPlusSize, d_header_size);
  } break;

  case HEADERTYPE_CHDR: {
    d_seq_num++;

    // Rollover at 12-bits
    if (d_seq_num > 0x0FFF)
      d_seq_num = 1;

    CHDR chdr;
    chdr.sid = d_port;
    chdr.length = d_payloadsize;
    chdr.seqPlusFlags = d_seq_num; // For now set all other flags to zero.
    memcpy((void *)d_tmpheaderbuff, (void *)&chdr, d_header_size);
  } break;
  }
}

int udp_sink_impl::work_test(int noutput_items,
                             gr_vector_const_void_star &input_items,
                             gr_vector_void_star &output_items) {
	  gr::thread::scoped_lock guard(d_setlock);

	  long numBytesToTransmit = noutput_items * d_block_size;
	  const char *in = (const char *)input_items[0];

	  // Build a long local queue to pull from so we can break it up easier
	  for (int i = 0; i < numBytesToTransmit; i++) {
	    d_localqueue->push_back(in[i]);
	  }

	  // Local boost buffer for transmitting
	  std::vector<boost::asio::const_buffer> transmitbuffer;

	  // Let's see how many blocks are in the buffer
	  int bytesAvailable = d_localqueue->size();
	  long blocksAvailable = bytesAvailable / d_precomp_datasize;

	  for (int curBlock = 0; curBlock < blocksAvailable; curBlock++) {
	    // Clear the next transmit buffer
	    transmitbuffer.clear();

	    // build our next header if we need it
	    if (d_header_type != HEADERTYPE_NONE) {
	      build_header();

	      transmitbuffer.push_back(
	          boost::asio::buffer((const void *)d_tmpheaderbuff, d_header_size));
	    }

	    // Fill the data buffer
	    for (int i = 0; i < d_precomp_datasize; i++) {
	      d_localbuffer[i] = d_localqueue->at(0);
	      d_localqueue->pop_front();
	    }

	    // Set up for transmit
	    transmitbuffer.push_back(
	        boost::asio::buffer((const void *)d_localbuffer, d_precomp_datasize));

	    // Send
	    d_udpsocket->send_to(transmitbuffer, d_endpoint);
	  }

	  int itemsreturned = blocksAvailable * d_precomp_data_overitemsize;
	  return itemsreturned;
}

int udp_sink_impl::work(int noutput_items,
                        gr_vector_const_void_star &input_items,
                        gr_vector_void_star &output_items) {
  gr::thread::scoped_lock guard(d_setlock);

  long numBytesToTransmit = noutput_items * d_block_size;
  const char *in = (const char *)input_items[0];

  // Build a long local queue to pull from so we can break it up easier
  for (int i = 0; i < numBytesToTransmit; i++) {
    d_localqueue->push_back(in[i]);
  }

  // Local boost buffer for transmitting
  std::vector<boost::asio::const_buffer> transmitbuffer;

  // Let's see how many blocks are in the buffer
  int bytesAvailable = d_localqueue->size();
  long blocksAvailable = bytesAvailable / d_precomp_datasize;

  for (int curBlock = 0; curBlock < blocksAvailable; curBlock++) {
    // Clear the next transmit buffer
    transmitbuffer.clear();

    // build our next header if we need it
    if (d_header_type != HEADERTYPE_NONE) {
      build_header();

      transmitbuffer.push_back(
          boost::asio::buffer((const void *)d_tmpheaderbuff, d_header_size));
    }

    // Fill the data buffer
    for (int i = 0; i < d_precomp_datasize; i++) {
      d_localbuffer[i] = d_localqueue->at(0);
      d_localqueue->pop_front();
    }

    // Set up for transmit
    transmitbuffer.push_back(
        boost::asio::buffer((const void *)d_localbuffer, d_precomp_datasize));

    // Send
    d_udpsocket->send_to(transmitbuffer, d_endpoint);
  }

  int itemsreturned = blocksAvailable * d_precomp_data_overitemsize;
  return itemsreturned;
}
} /* namespace grnet */
} /* namespace gr */
