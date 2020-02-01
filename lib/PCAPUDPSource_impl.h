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

#ifndef INCLUDED_GRNET_PCAPUDPSOURCE_IMPL_H
#define INCLUDED_GRNET_PCAPUDPSOURCE_IMPL_H

#include <grnet/PCAPUDPSource.h>
#include <queue>

#include "packet_headers.h"
#include <boost/thread/thread.hpp>
#include <pcap/pcap.h>

namespace gr {
namespace grnet {

class PCAPUDPSource_impl : public PCAPUDPSource {
private:
  size_t d_itemsize;
  size_t d_veclen;
  size_t d_block_size;

  bool d_notifyMissed;

  int d_port;
  int d_header_type;
  int d_header_size;
  uint16_t d_payloadsize;
  int d_precompDataSize;
  int d_precompDataOverItemSize;

  uint64_t d_seq_num;
  unsigned char *localBuffer;

  std::queue<unsigned char> localQueue;
  std::queue<unsigned char> netQueue;

  std::string d_filename;
  bool d_repeat;

  boost::mutex d_mutex;
  boost::mutex fp_mutex;
  boost::mutex d_netQueueMutex;

  pcap_t *pcapFile;

  boost::thread *readThread = NULL;
  bool threadRunning;
  bool stopThread;

  void runThread();

  void open();
  void close();
  void restart();

  uint64_t getHeaderSeqNum();

public:
  PCAPUDPSource_impl(size_t itemsize, int port, int headerType, int payloadsize,
                     bool notifyMissed, const char *filename, bool repeat);
  ~PCAPUDPSource_impl();

  bool stop();

  size_t dataAvailable();
  inline size_t netDataAvailable();

  // Where all the action really happens
  int work(int noutput_items, gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
};

} // namespace grnet
} // namespace gr

#endif /* INCLUDED_GRNET_PCAPUDPSOURCE_IMPL_H */
