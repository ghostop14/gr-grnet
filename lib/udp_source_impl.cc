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

#include "udp_source_impl.h"
#include <gnuradio/io_signature.h>
#include <sstream>

namespace gr {
namespace grnet {

udp_source::sptr udp_source::make(size_t itemsize, size_t vecLen, int port,
                                  int headerType, int payloadsize,
                                  long udp_recv_buf_size, bool notifyMissed,
                                  bool sourceZeros, bool ipv6) {
  return gnuradio::get_initial_sptr(
      new udp_source_impl(itemsize, vecLen, port, headerType, payloadsize,
                          udp_recv_buf_size, notifyMissed, sourceZeros, ipv6));
}

/*
 * The private constructor
 */
udp_source_impl::udp_source_impl(size_t itemsize, size_t vecLen, int port,
                                 int headerType, int payloadsize,
                                 long udp_recv_buf_size, bool notifyMissed,
                                 bool sourceZeros, bool ipv6)
    : gr::sync_block("udp_source", gr::io_signature::make(0, 0, 0),
                     gr::io_signature::make(1, 1, itemsize * vecLen)) {

  d_udp_recv_buf_size = udp_recv_buf_size;

  is_ipv6 = ipv6;

  d_itemsize = itemsize;
  d_veclen = vecLen;

  d_block_size = d_itemsize * d_veclen;
  d_port = port;
  d_seq_num = 0;
  d_notifyMissed = notifyMissed;
  d_sourceZeros = sourceZeros;
  d_header_type = headerType;

  d_payloadsize = payloadsize;
  d_partialFrameCounter = 0;

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

  case HEADERTYPE_OLDATA:
    d_header_size = sizeof(OldATAHeader);
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

  d_precompDataSize = d_payloadsize - d_header_size;
  d_precompDataOverItemSize = d_precompDataSize / d_itemsize;

  localBuffer = new unsigned char[d_payloadsize];
  long maxCircBuffer;

  // Let's keep it from getting too big
  if (d_payloadsize < 2000) {
    maxCircBuffer = d_payloadsize * 4000;
  } else {
    if (d_payloadsize < 5000)
      maxCircBuffer = d_payloadsize * 2000;
    else
      maxCircBuffer = d_payloadsize * 1500;
  }

  d_localqueue = new boost::circular_buffer<unsigned char>(maxCircBuffer);

  if (is_ipv6)
    d_endpoint =
        boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v6(), port);
  else
    d_endpoint =
        boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port);

  try {
    d_udpsocket = new boost::asio::ip::udp::socket(d_io_service, d_endpoint);
  } catch (const std::exception &ex) {
    throw std::runtime_error(std::string("[UDP Source] Error occurred: ") +
                             ex.what());
  }

  // Make sure we have a big enough buffer
  boost::asio::socket_base::receive_buffer_size option(d_udp_recv_buf_size);
  d_udpsocket->set_option(option);

  int outMultiple = (d_payloadsize - d_header_size) / d_block_size;

  if (outMultiple == 1)
    outMultiple = 2; // Ensure we get pairs, for instance complex -> ichar pairs

  std::stringstream msg_stream;
  msg_stream << "Listening for data on UDP port " << port << ".";
  GR_LOG_INFO(d_logger, msg_stream.str());

  gr::block::set_output_multiple(outMultiple);
}

/*
 * Our virtual destructor.
 */
udp_source_impl::~udp_source_impl() { stop(); }

bool udp_source_impl::stop() {
  if (d_udpsocket) {
    d_udpsocket->close();

    d_udpsocket = NULL;

    d_io_service.reset();
    d_io_service.stop();
  }

  if (localBuffer) {
    delete[] localBuffer;
    localBuffer = NULL;
  }

  if (d_localqueue) {
    delete d_localqueue;
    d_localqueue = NULL;
  }
  return true;
}

size_t udp_source_impl::data_available() {
  // Get amount of data available
  boost::asio::socket_base::bytes_readable command(true);
  d_udpsocket->io_control(command);
  size_t bytes_readable = command.get();

  return (bytes_readable + d_localqueue->size());
}

size_t udp_source_impl::netdata_available() {
  // Get amount of data available
  boost::asio::socket_base::bytes_readable command(true);
  d_udpsocket->io_control(command);
  size_t bytes_readable = command.get();

  return bytes_readable;
}

uint64_t udp_source_impl::get_header_seqnum() {
  uint64_t retVal = 0;

  switch (d_header_type) {
  case HEADERTYPE_SEQNUM: {
    retVal = ((HeaderSeqNum *)localBuffer)->seqnum;
  } break;

  case HEADERTYPE_SEQPLUSSIZE: {
    retVal = ((HeaderSeqPlusSize *)localBuffer)->seqnum;
  } break;

  case HEADERTYPE_CHDR: {
    // Rollover at 12-bits
    if (d_seq_num > 0x0FFF)
      d_seq_num = 1;

    retVal = ((CHDR *)localBuffer)->seqPlusFlags & 0x0FFF;
  } break;

  case HEADERTYPE_OLDATA: {
    retVal = ((OldATAHeader *)localBuffer)->seq;
  } break;
  }

  return retVal;
}

int udp_source_impl::work_test(int noutput_items,
                               gr_vector_const_void_star &input_items,
                               gr_vector_void_star &output_items) {
  gr::thread::scoped_lock guard(d_setlock);

  static bool firstTime = true;
  // static int testCount=0;
  static int underRunCounter = 0;

  int bytesAvailable = netdata_available();
  unsigned char *out = (unsigned char *)output_items[0];
  unsigned int numRequested = noutput_items * d_block_size;

  // quick exit if nothing to do
  if ((bytesAvailable == 0) && (d_localqueue->size() == 0)) {
    if (underRunCounter == 0) {
      if (!firstTime) {
        std::cout << "nU";
      } else
        firstTime = false;
    } else {
      if (underRunCounter > 100)
        underRunCounter = 0;
    }

    underRunCounter++;
    if (d_sourceZeros) {
      // Just return 0's
      memset((void *)out, 0x00, numRequested);
      return noutput_items;
    } else {
      return 0;
    }
  }

  int bytesRead;
  int localNumItems;

  // we could get here even if no data was received but there's still data in
  // the queue. however read blocks so we want to make sure we have data before
  // we call it.
  if (bytesAvailable > 0) {
    boost::asio::streambuf::mutable_buffers_type buf =
        d_read_buffer.prepare(bytesAvailable);
    // http://stackoverflow.com/questions/28929699/boostasio-read-n-bytes-from-socket-to-streambuf
    bytesRead = d_udpsocket->receive_from(buf, d_endpoint);

    if (bytesRead > 0) {
      d_read_buffer.commit(bytesRead);

      // Get the data and add it to our local queue.  We have to maintain a
      // local queue in case we read more bytes than noutput_items is asking
      // for.  In that case we'll only return noutput_items bytes
      const char *readData =
          boost::asio::buffer_cast<const char *>(d_read_buffer.data());
      for (int i = 0; i < bytesRead; i++) {
        d_localqueue->push_back(readData[i]);
      }
      d_read_buffer.consume(bytesRead);
    }
  }

  if (d_localqueue->size() < d_payloadsize) {
    // we don't have sufficient data for a block yet.
    return 0; // Don't memset 0x00 since we're starting to get data.  In this
              // case we'll hold for the rest.
  }

  // Now if we're here we should have at least 1 block.

  // let's figure out how much we have in relation to noutput_items, accounting
  // for headers

  // Number of data-only blocks requested (set_output_multiple() should make
  // sure this is an integer multiple)
  long blocksRequested = noutput_items / d_precompDataOverItemSize;
  // Number of blocks available accounting for the header as well.
  long blocksAvailable = d_localqueue->size() / (d_payloadsize);
  long blocksRetrieved;
  int itemsreturned;

  if (blocksRequested <= blocksAvailable)
    blocksRetrieved = blocksRequested;
  else
    blocksRetrieved = blocksAvailable;

  // items returned is going to match the payload (actual data) of the number of
  // blocks.
  itemsreturned = blocksRetrieved * d_precompDataOverItemSize;

  // We're going to have to read the data out in blocks, account for the header,
  // then just move the data part into the out[] array.

  unsigned char *pData;
  pData = &localBuffer[d_header_size];
  int outIndex = 0;
  int skippedPackets = 0;

  for (int curPacket = 0; curPacket < blocksRetrieved; curPacket++) {
    // Move a packet to our local buffer
    for (int curByte = 0; curByte < d_payloadsize; curByte++) {
      localBuffer[curByte] = d_localqueue->at(0); // d_localqueue.front();
                                                  // d_localqueue.pop();
      d_localqueue->pop_front();
    }

    // Interpret the header if present
    if (d_header_type != HEADERTYPE_NONE) {
      uint64_t pktSeqNum = get_header_seqnum();

      if (d_seq_num > 0) { // d_seq_num will be 0 when this block starts
        if (pktSeqNum > d_seq_num) {
          // Ideally pktSeqNum = d_seq_num + 1.  Therefore this should do += 0
          // when no packets are dropped.
          skippedPackets += pktSeqNum - d_seq_num - 1;
        }

        // Store as current for next pass.
        d_seq_num = pktSeqNum;
      } else {
        // just starting.  Prime it for no loss on the first packet.
        d_seq_num = pktSeqNum;
      }
    }

    // Move the data to the output buffer and increment the out index
    memcpy(&out[outIndex], pData, d_precompDataSize);
    outIndex = outIndex + d_precompDataSize;
  }

  if (skippedPackets > 0 && d_notifyMissed) {
    std::stringstream msg_stream;
    msg_stream << "[UDP source:" << d_port
               << "] missed  packets: " << skippedPackets;
    GR_LOG_WARN(d_logger, msg_stream.str());
  }

  // firstTime = false;

  // If we had less data than requested, it'll be reflected in the return value.
  return itemsreturned;
}

int udp_source_impl::work(int noutput_items,
                          gr_vector_const_void_star &input_items,
                          gr_vector_void_star &output_items) {
  gr::thread::scoped_lock guard(d_setlock);

  static bool firstTime = true;
  static int underRunCounter = 0;

  int bytesAvailable = netdata_available();
  unsigned char *out = (unsigned char *)output_items[0];
  unsigned int numRequested = noutput_items * d_block_size;

  // quick exit if nothing to do
  if ((bytesAvailable == 0) && (d_localqueue->size() == 0)) {
    underRunCounter++;
    d_partialFrameCounter = 0;
    if (d_sourceZeros) {
      // Just return 0's
      memset((void *)out, 0x00, numRequested); // numRequested will be in bytes
      return noutput_items;
    } else {
      if (underRunCounter == 0) {
        if (!firstTime) {
          std::cout << "nU";
        } else
          firstTime = false;
      } else {
        if (underRunCounter > 100)
          underRunCounter = 0;
      }

      return 0;
    }
  }

  int bytesRead;
  int localNumItems;

  // we could get here even if no data was received but there's still data in
  // the queue. however read blocks so we want to make sure we have data before
  // we call it.
  if (bytesAvailable > 0) {
    boost::asio::streambuf::mutable_buffers_type buf =
        d_read_buffer.prepare(bytesAvailable);
    // http://stackoverflow.com/questions/28929699/boostasio-read-n-bytes-from-socket-to-streambuf
    bytesRead = d_udpsocket->receive_from(buf, d_endpoint);

    if (bytesRead > 0) {
      d_read_buffer.commit(bytesRead);

      // Get the data and add it to our local queue.  We have to maintain a
      // local queue in case we read more bytes than noutput_items is asking
      // for.  In that case we'll only return noutput_items bytes
      const char *readData =
          boost::asio::buffer_cast<const char *>(d_read_buffer.data());
      for (int i = 0; i < bytesRead; i++) {
        d_localqueue->push_back(readData[i]);
      }
      d_read_buffer.consume(bytesRead);
    }
  }

  if (d_localqueue->size() < d_payloadsize) {
    // since we should be getting these in UDP packet blocks matched on the
    // sender/receiver, this should be a fringe case, or a case where another
    // app is sourcing the packets.
    d_partialFrameCounter++;

    if (d_partialFrameCounter >= 100) {
      std::stringstream msg_stream;
      msg_stream << "Insufficient block data.  Check your sending "
                    "app is using "
                 << d_payloadsize << " send blocks.";
      GR_LOG_WARN(d_logger, msg_stream.str());

      // This is just a safety to clear in the case there's a hanging partial
      // packet. If we've lingered through a number of calls and we still don't
      // have any data, clear the stale data.
      while (d_localqueue->size() > 0)
        d_localqueue->pop_front();

      d_partialFrameCounter = 0;
    }
    return 0; // Don't memset 0x00 since we're starting to get data.  In this
              // case we'll hold for the rest.
  }

  // If we're here, it's not a partial hanging frame
  d_partialFrameCounter = 0;

  // Now if we're here we should have at least 1 block.

  // let's figure out how much we have in relation to noutput_items, accounting
  // for headers

  // Number of data-only blocks requested (set_output_multiple() should make
  // sure this is an integer multiple)
  long blocksRequested = noutput_items / d_precompDataOverItemSize;
  // Number of blocks available accounting for the header as well.
  long blocksAvailable = d_localqueue->size() / (d_payloadsize);
  long blocksRetrieved;
  int itemsreturned;

  if (blocksRequested <= blocksAvailable)
    blocksRetrieved = blocksRequested;
  else
    blocksRetrieved = blocksAvailable;

  // items returned is going to match the payload (actual data) of the number of
  // blocks.
  itemsreturned = blocksRetrieved * d_precompDataOverItemSize;

  // We're going to have to read the data out in blocks, account for the header,
  // then just move the data part into the out[] array.

  unsigned char *pData;
  pData = &localBuffer[d_header_size];
  int outIndex = 0;
  int skippedPackets = 0;

  for (int curPacket = 0; curPacket < blocksRetrieved; curPacket++) {
    // Move a packet to our local buffer
    for (int curByte = 0; curByte < d_payloadsize; curByte++) {
      localBuffer[curByte] = d_localqueue->at(0);
      d_localqueue->pop_front();
    }

    // Interpret the header if present
    if (d_header_type != HEADERTYPE_NONE) {
      uint64_t pktSeqNum = get_header_seqnum();

      if (d_seq_num > 0) { // d_seq_num will be 0 when this block starts
        if (pktSeqNum > d_seq_num) {
          // Ideally pktSeqNum = d_seq_num + 1.  Therefore this should do += 0
          // when no packets are dropped.
          skippedPackets += pktSeqNum - d_seq_num - 1;
        }

        // Store as current for next pass.
        d_seq_num = pktSeqNum;
      } else {
        // just starting.  Prime it for no loss on the first packet.
        d_seq_num = pktSeqNum;
      }
    }

    // Move the data to the output buffer and increment the out index
    memcpy(&out[outIndex], pData, d_precompDataSize);
    outIndex = outIndex + d_precompDataSize;
  }

  if (skippedPackets > 0 && d_notifyMissed) {
    std::stringstream msg_stream;
    msg_stream << "[UDP source:" << d_port
               << "] missed  packets: " << skippedPackets;
    GR_LOG_WARN(d_logger, msg_stream.str());
  }

  // If we had less data than requested, it'll be reflected in the return value.
  return itemsreturned;
}
} /* namespace grnet */
} /* namespace gr */
