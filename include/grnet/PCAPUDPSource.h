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

#ifndef INCLUDED_GRNET_PCAPUDPSOURCE_H
#define INCLUDED_GRNET_PCAPUDPSOURCE_H

#include <gnuradio/sync_block.h>
#include <grnet/api.h>

namespace gr {
namespace grnet {

/*!
 * \brief <+description of block+>
 * \ingroup grnet
 *
 */
class GRNET_API PCAPUDPSource : virtual public gr::sync_block {
public:
  typedef std::shared_ptr<PCAPUDPSource> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of grnet::PCAPUDPSource.
   *
   * To avoid accidental use of raw pointers, grnet::PCAPUDPSource's
   * constructor is in a private implementation
   * class. grnet::PCAPUDPSource::make is the public interface for
   * creating new instances.
   */
  static sptr make(size_t itemsize, int port, int headerType, int payloadsize,
                   bool notifyMissed, const char *filename, bool repeat);
};

} // namespace grnet
} // namespace gr

#endif /* INCLUDED_GRNET_PCAPUDPSOURCE_H */
