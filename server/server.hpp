#pragma once

#include "connection.hpp"

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <thread>
#include <memory>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <string>
#include <utility>
#include <optional>

using namespace boost::asio::ip;

namespace miniredis
{

  class server final
  {
  public:
    server (const server&) = delete;
    server& operator=(const server&) = delete;

    explicit server (const std::string& address, const std::string& port);
    void run ();

  
  private:
    void start_accept ();
    void await_stop ();

    bool stopped_;
    bool closed_ = false;
    
    boost::asio::io_context io_context_;
    boost::asio::signal_set signals_;
    tcp::acceptor acceptor_;
    connection_manager manager_;
  };
}
