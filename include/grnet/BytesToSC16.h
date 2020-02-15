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

#ifndef INCLUDED_GRNET_BYTESTOSC16_H
#define INCLUDED_GRNET_BYTESTOSC16_H

#include <gnuradio/sync_decimator.h>
#include <grnet/api.h>

namespace gr {
namespace grnet {

/*!
 * \brief <+description of block+>
 * \ingroup grnet
 *
 */
class GRNET_API BytesToSC16 : virtual public gr::sync_decimator {
public:
  typedef boost::shared_ptr<BytesToSC16> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of grnet::BytesToSC16.
   *
   * To avoid accidental use of raw pointers, grnet::BytesToSC16's
   * constructor is in a private implementation
   * class. grnet::BytesToSC16::make is the public interface for
   * creating new instances.
   */
  static sptr make();
};

} // namespace grnet
} // namespace gr

#endif /* INCLUDED_GRNET_BYTESTOSC16_H */
