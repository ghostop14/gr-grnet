/* -*- c++ -*- */
/*
 * Copyright 2019 ghostop14.
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

#include "PCAPUDPSource_impl.h"
#include <gnuradio/io_signature.h>

#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <sstream>
#include <unistd.h>

namespace gr {
namespace grnet {

PCAPUDPSource::sptr PCAPUDPSource::make(size_t itemsize, int port,
                                        int headerType, int payloadsize,
                                        bool notifyMissed, const char *filename,
                                        bool repeat) {
  return gnuradio::get_initial_sptr(new PCAPUDPSource_impl(
      itemsize, port, headerType, payloadsize, notifyMissed, filename, repeat));
}

/*
 * The private constructor
 */
PCAPUDPSource_impl::PCAPUDPSource_impl(size_t itemsize, int port,
                                       int headerType, int payloadsize,
                                       bool notifyMissed, const char *filename,
                                       bool repeat)
    : gr::sync_block("PCAPUDPSource", gr::io_signature::make(0, 0, 0),
                     gr::io_signature::make(1, 1, itemsize)) {
  d_itemsize = itemsize;
  d_veclen = 1;
  d_filename = filename;
  d_repeat = repeat;

  d_block_size = d_itemsize * d_veclen;
  d_port = port;
  d_seq_num = 0;
  d_notifyMissed = notifyMissed;
  d_header_type = headerType;
  ;
  d_payloadsize = payloadsize;

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
    throw std::runtime_error("[PCAP UDP Source] Unknown header type");
    break;
  }

  if (d_payloadsize < 8) {
    GR_LOG_ERROR(d_logger,
                 "Payload size is too small.  Must be at "
                 "least 8 bytes once header/trailer adjustments are made.");
    throw std::runtime_error("[PCAP UDP Source] Payload size is too small.");
    exit(1);
  }

  d_precompDataSize = d_payloadsize - d_header_size;
  d_precompDataOverItemSize = d_precompDataSize / d_itemsize;

  localBuffer = new unsigned char[d_payloadsize];

  int outMultiple = (d_payloadsize - d_header_size) / d_block_size;

  if (outMultiple == 1)
    outMultiple = 2; // Ensure we get pairs, for instance complex -> ichar pairs

  std::stringstream msg;
  msg << "[PCAP UDP Source] Using " << d_filename;
  GR_LOG_INFO(d_logger, msg.str());

  gr::block::set_output_multiple(outMultiple);

  threadRunning = false;
  stopThread = false;

  if (FILE *file = fopen(d_filename.c_str(), "r")) {
    fclose(file);
  } else {
    perror(filename);
    throw std::runtime_error("[PCAP UDP Source] can't open file");
    return;
  }

  pcapFile = NULL;

  readThread =
      new boost::thread(boost::bind(&PCAPUDPSource_impl::runThread, this));
}

/*
 * Our virtual destructor.
 */
PCAPUDPSource_impl::~PCAPUDPSource_impl() { stop(); }

void PCAPUDPSource_impl::runThread() {
  threadRunning = true;
  int maxQueueSize = 64000;

  int sizeUDPHeader = sizeof(struct udphdr);
  pcap_pkthdr header;
  const u_char *p;
  timeval tv;
  unsigned long matchingPackets;

  while (!stopThread) {
    tv = {0, 0};
    restart();
    matchingPackets = 0;

    while ((p = pcap_next(pcapFile, &header))) {
      if (header.len != header.caplen) {
        continue;
      }
      auto eth = reinterpret_cast<const ether_header *>(p);

      // jump over and ignore vlan tag
      if (ntohs(eth->ether_type) == ETHERTYPE_VLAN) {
        p += 4;
        eth = reinterpret_cast<const ether_header *>(p);
      }
      if (ntohs(eth->ether_type) != ETHERTYPE_IP) {
        continue;
      }
      auto ip = reinterpret_cast<const iphdr *>(p + sizeof(ether_header));
      if (ip->version != 4) {
        continue;
      }
      if (ip->protocol != IPPROTO_UDP) {
        continue;
      }

      // IP Header length is defined in a packet field (IHL).  IHL represents
      // the # of 32-bit words So header size is ihl * 4 [bytes]
      int etherIPHeaderSize = sizeof(ether_header) + ip->ihl * 4;

      auto udp = reinterpret_cast<const udphdr *>(p + etherIPHeaderSize);
      if (tv.tv_sec == 0) {
        tv = header.ts;
      }
      timeval diff;
      timersub(&header.ts, &tv, &diff);
      tv = header.ts;

      /* This delay was causing too much slow-down.
      unsigned int delay_as_micro = diff.tv_sec*1000000 + diff.tv_usec; // (diff.tv_sec + diff.tv_usec / 1e6)*1e6;

      if (delay_as_micro > 0)
        usleep(delay_as_micro/1000); // there's lots of work below, so full sleep from the packet is too long.
	  */

      uint16_t destPort = ntohs(udp->dest);

      if (destPort == d_port) {
        matchingPackets++;
        size_t len = ntohs(udp->len) - sizeUDPHeader;

        if (len > 0) {
          const u_char *pData = &p[etherIPHeaderSize + sizeUDPHeader];

          gr::thread::scoped_lock guard(d_netQueueMutex);

          for (int i = 0; i < len; i++) {
            netQueue.push(pData[i]);
          }
        }
      }


      // Monitor that we don't get too backed up.
      while (!stopThread && netDataAvailable() > maxQueueSize) {
        // Just in case we're running fast, let the system catch up.
        usleep(20);
      }

      if (stopThread)
        break;
    }

    if (matchingPackets == 0) {
      std::cerr << "[PCAP UDP Source] End of file reached and no matching "
                   "packets.  Do you have the right port number?"
                << std::endl;
    }

    while (!stopThread && netDataAvailable() > d_payloadsize) {
      // Just in case we're running fast, let the system catch up.
      usleep(1000);
    }

    // end if we're not repeating.
    if (d_repeat) {
      std::cerr << "[PCAP UDP Source] End of file reached.  Repeating."
                << std::endl;
    } else {
      std::cerr << "[PCAP UDP Source] End of file reached." << std::endl;
      break;
    }
  }

  threadRunning = false;
}

void PCAPUDPSource_impl::restart() {
  close();
  open();
}

void PCAPUDPSource_impl::open() {
  gr::thread::scoped_lock lock(fp_mutex);
  char errbuf[PCAP_ERRBUF_SIZE];
  try {
    pcapFile = pcap_open_offline(d_filename.c_str(), errbuf);

    if (pcapFile == NULL) {
      std::cerr << "[PCAP UDP Source] Error occurred: " << errbuf << std::endl;
      return;
    }
  } catch (const std::exception &ex) {
    std::cerr << "[PCAP UDP Source] Error occurred: " << ex.what() << std::endl;
    pcapFile = NULL;
  }
}

void PCAPUDPSource_impl::close() {
  gr::thread::scoped_lock lock(fp_mutex);
  if (pcapFile != NULL) {
    pcap_close(pcapFile);
    pcapFile = NULL;
  }
}

bool PCAPUDPSource_impl::stop() {
  if (readThread) {
    stopThread = true;

    while (threadRunning) {
      usleep(10000); // sleep 10 millisec
    }

    delete readThread;
    readThread = NULL;
  }

  close();

  if (localBuffer) {
    delete[] localBuffer;
    localBuffer = NULL;
  }

  return true;
}

size_t PCAPUDPSource_impl::dataAvailable() {
  return (netQueue.size() + localQueue.size());
}

size_t PCAPUDPSource_impl::netDataAvailable() {
  // Get amount of data available
  gr::thread::scoped_lock guard(d_netQueueMutex);
  size_t size = netQueue.size();

  return size;
}

uint64_t PCAPUDPSource_impl::getHeaderSeqNum() {
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

int PCAPUDPSource_impl::work(int noutput_items,
                             gr_vector_const_void_star &input_items,
                             gr_vector_void_star &output_items) {
  gr::thread::scoped_lock guard(d_mutex);

  static bool firstTime = true;
  static int testCount = 0;
  static int underRunCounter = 0;

  int bytesAvailable = netDataAvailable();
  unsigned char *out = (unsigned char *)output_items[0];
  unsigned int numRequested = noutput_items * d_block_size;

  // quick exit if nothing to do
  if ((bytesAvailable == 0) && (localQueue.size() == 0)) {
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
    return 0;
  }

  int bytesRead;
  int localNumItems;

  // we could get here even if no data was received but there's still data in
  // the queue. however read blocks so we want to make sure we have data before
  // we call it.
  if (bytesAvailable > 0) {
    bytesRead = bytesAvailable;

    if (bytesRead > 0) {
      // Get the data and add it to our local queue.  We have to maintain a
      // local queue in case we read more bytes than noutput_items is asking
      // for.  In that case we'll only return noutput_items bytes
      gr::thread::scoped_lock guard(d_netQueueMutex);
      for (int i = 0; i < bytesRead; i++) {
        localQueue.push(netQueue.front());
        netQueue.pop();
      }
    }
  }

  if (localQueue.size() < d_payloadsize) {
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
  long blocksAvailable = localQueue.size() / (d_payloadsize);
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
      localBuffer[curByte] = localQueue.front();
      localQueue.pop();
    }

    // Interpret the header if present
    if (d_header_type != HEADERTYPE_NONE) {
      uint64_t pktSeqNum = getHeaderSeqNum();

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
    std::stringstream msg;
    msg << "[UDP Sink port " << d_port
        << "] missed  packets: " << skippedPackets;
    GR_LOG_WARN(d_logger, msg.str());
  }

  // If we had less data than requested, it'll be reflected in the return value.
  return itemsreturned;
}

} /* namespace grnet */
} /* namespace gr */
