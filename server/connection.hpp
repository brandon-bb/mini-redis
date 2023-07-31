#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>
#include <set>
#include <memory>
#include <string>

using namespace boost::asio::ip;

namespace miniredis
{

class connection_manager;

class connection final : public std::enable_shared_from_this<connection>
{
private:
  void async_read ();
  void async_write ();

  tcp::socket socket_;
  std::string message_;
  connection_manager& connection_manager_;


public:
  connection (const connection&) = delete;
  connection& operator=(const connection&) = delete;

  explicit connection (tcp::socket socket, connection_manager& manager);
  void start ();
  void stop ();

  using connection_pointer = std::shared_ptr <connection>;

  tcp::socket& socket () { return socket_; }
  
  
};

class connection_manager
{
public:
  connection_manager (const connection_manager&) = delete;
  connection_manager& operator=(const connection_manager&) = delete;

  connection_manager ();
  void start (connection::connection_pointer c);
  void stop (connection::connection_pointer c);
  void stop_all ();

private:
  std::set <connection::connection_pointer> connections_;
};
}