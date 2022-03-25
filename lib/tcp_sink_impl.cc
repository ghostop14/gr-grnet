/* -*- c++ -*- */
/*
 * Copyright 2017,2020 ghostop14.
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

#include "tcp_sink_impl.h"
#include <gnuradio/io_signature.h>
#include <boost/format.hpp>
#include <sstream>

namespace gr {
namespace grnet {

tcp_sink::sptr tcp_sink::make(size_t itemsize, size_t vecLen,
                              const std::string &host, int port, int sinkMode) {
  return gnuradio::get_initial_sptr(
      new tcp_sink_impl(itemsize, vecLen, host, port, sinkMode));
}

/*
 * The private constructor
 */
tcp_sink_impl::tcp_sink_impl(size_t itemsize, size_t vecLen,
                             const std::string &host, int port, int sinkMode)
    : gr::sync_block("tcp_sink",
                     gr::io_signature::make(1, 1, itemsize * vecLen),
                     gr::io_signature::make(0, 0, 0)),
      d_itemsize(itemsize), d_veclen(vecLen), d_port(port), d_host(host),
      d_sinkmode(sinkMode), d_thread_running(false), d_stop_thread(false),
      d_listener_thread(NULL), d_start_new_listener(false),
      d_initial_connection(true) {
  d_block_size = d_itemsize * d_veclen;

  if (d_sinkmode == TCPSINKMODE_CLIENT) {
    // In this mode, we're connecting to a remote TCP service listener
    // as a client.
    std::stringstream msg;

    msg << "[TCP Sink] connecting to " << host << " on port " << port;
    GR_LOG_INFO(d_logger, msg.str());

    boost::system::error_code err;
    d_tcpsocket = new boost::asio::ip::tcp::socket(d_io_service);

    std::string s_port = (boost::format("%d") % port).str();
    boost::asio::ip::tcp::resolver resolver(d_io_service);
    boost::asio::ip::tcp::resolver::query query(
        d_host, s_port, boost::asio::ip::resolver_query_base::passive);

    d_endpoint = *resolver.resolve(query, err);

    if (err) {
      throw std::runtime_error(
          std::string("[TCP Sink] Unable to resolve host/IP: ") +
          err.message());
    }

    if (d_host.find(":") != std::string::npos)
      is_ipv6 = true;
    else {
      // This block supports a check that a name rather than an IP is provided.
      // the endpoint is then checked after the resolver is done.
      if (d_endpoint.address().is_v6())
        is_ipv6 = true;
      else
        is_ipv6 = false;
    }

    d_tcpsocket->connect(d_endpoint, err);
    if (err) {
      throw std::runtime_error(std::string("[TCP Sink] Connection error: ") +
                               err.message());
    }

    d_connected = true;

    boost::asio::socket_base::keep_alive option(true);
    d_tcpsocket->set_option(option);
  } else {
    // In this mode, we're starting a local port listener and waiting
    // for inbound connections.
    d_start_new_listener = true;
    d_listener_thread =
        new boost::thread(boost::bind(&tcp_sink_impl::run_listener, this));
  }
}

void tcp_sink_impl::run_listener() {
  d_thread_running = true;

  while (!d_stop_thread) {
    // this will block
    if (d_start_new_listener) {
      d_start_new_listener = false;
      connect(d_initial_connection);
      d_initial_connection = false;
    } else
      usleep(10);
  }

  d_thread_running = false;
}

void tcp_sink_impl::accept_handler(boost::asio::ip::tcp::socket *new_connection,
                                   const boost::system::error_code &error) {
  if (!error) {
    GR_LOG_INFO(d_logger, "Client connection received.");

    // Accept succeeded.
    d_tcpsocket = new_connection;

    boost::asio::socket_base::keep_alive option(true);
    d_tcpsocket->set_option(option);
    d_connected = true;

  } else {
    std::stringstream msg;
    msg << "Error code " << error << " accepting TCP session.";
    GR_LOG_ERROR(d_logger, msg.str());

    // Boost made a copy so we have to clean up
    delete new_connection;

    // safety settings.
    d_connected = false;
    d_tcpsocket = NULL;
  }
}

void tcp_sink_impl::connect(bool initial_connection) {
  std::stringstream msg;
  msg << "Waiting for connection on port " << d_port;
  GR_LOG_INFO(d_logger, msg.str());

  if (initial_connection) {
    if (is_ipv6)
      d_acceptor = new boost::asio::ip::tcp::acceptor(
          d_io_service,
          boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), d_port));
    else
      d_acceptor = new boost::asio::ip::tcp::acceptor(
          d_io_service,
          boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), d_port));
  } else {
    d_io_service.reset();
  }

  if (d_tcpsocket) {
    delete d_tcpsocket;
  }
  d_tcpsocket = NULL;
  d_connected = false;

  boost::asio::ip::tcp::socket *tmpSocket =
      new boost::asio::ip::tcp::socket(d_io_service);
  d_acceptor->async_accept(
      *tmpSocket, boost::bind(&tcp_sink_impl::accept_handler, this, tmpSocket,
                              boost::asio::placeholders::error));

  if (initial_connection) {
    d_io_service.run();
  } else {
    d_io_service.run();
  }
}

/*
 * Our virtual destructor.
 */
tcp_sink_impl::~tcp_sink_impl() { stop(); }

bool tcp_sink_impl::stop() {
  if (d_thread_running) {
    d_stop_thread = true;
  }

  if (d_tcpsocket) {
    d_tcpsocket->close();
    delete d_tcpsocket;
    d_tcpsocket = NULL;
  }

  d_io_service.reset();
  d_io_service.stop();

  if (d_acceptor) {
    delete d_acceptor;
    d_acceptor = NULL;
  }

  if (d_listener_thread) {
    while (d_thread_running)
      usleep(5);

    delete d_listener_thread;
    d_listener_thread = NULL;
  }

  return true;
}

int tcp_sink_impl::work_test(int noutput_items,
                             gr_vector_const_void_star &input_items,
                             gr_vector_void_star &output_items) {
  gr::thread::scoped_lock guard(d_setlock);

  const char *in = (const char *)input_items[0];
  unsigned int noi = noutput_items * d_block_size;
  int bytesWritten;
  int bytesRemaining = noi;

  ec.clear();

  while ((bytesRemaining > 0) && (!ec)) {
    bytesWritten = boost::asio::write(
        *d_tcpsocket, boost::asio::buffer((const void *)in, noi), ec);
    bytesRemaining -= bytesWritten;
  }

  return noutput_items;
}

void tcp_sink_impl::check_for_disconnect() {
  int bytesRead;

  char buff[1];
  bytesRead = d_tcpsocket->receive(boost::asio::buffer(buff),
                                   d_tcpsocket->message_peek, ec);
  if ((boost::asio::error::eof == ec) ||
      (boost::asio::error::connection_reset == ec)) {
    std::stringstream msg;
    msg << "Disconnect detected on " << d_host << ":" << d_port << ".";
    GR_LOG_INFO(d_logger, msg.str());

    d_tcpsocket->close();
    delete d_tcpsocket;
    d_tcpsocket = NULL;

    exit(1);
  } else {
    if (ec) {
      std::stringstream msg;
      msg << "Socket error " << ec << " detected.";
      GR_LOG_ERROR(d_logger, msg.str());
    }
  }
}

int tcp_sink_impl::work(int noutput_items,
                        gr_vector_const_void_star &input_items,
                        gr_vector_void_star &output_items) {
  gr::thread::scoped_lock guard(d_setlock);

  if (!d_connected)
    return noutput_items;

  const char *in = (const char *)input_items[0];
  unsigned int noi = noutput_items * d_block_size;
  int bytesWritten;
  int bytesRemaining = noi;

  ec.clear();

  char *pBuff;
  pBuff = (char *)input_items[0];

  while ((bytesRemaining > 0) && (!ec)) {
    bytesWritten = boost::asio::write(
        *d_tcpsocket, boost::asio::buffer((const void *)pBuff, bytesRemaining),
        ec);
    bytesRemaining -= bytesWritten;
    pBuff += bytesWritten;

    if (ec == boost::asio::error::connection_reset ||
        ec == boost::asio::error::broken_pipe) {

      // Connection was reset
      d_connected = false;
      bytesRemaining = 0;

      if (d_sinkmode == TCPSINKMODE_CLIENT) {
        GR_LOG_WARN(d_logger,
                    "Server closed the connection.  Stopping processing.");

        return WORK_DONE;
      } else {
        GR_LOG_INFO(d_logger,
                    "Client disconnected. Waiting for new connection.");

        // start waiting for another connection
        d_start_new_listener = true;
      }
    }
  }

  return noutput_items;
}
} /* namespace grnet */
} /* namespace gr */
