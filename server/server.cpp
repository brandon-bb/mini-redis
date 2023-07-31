#include "server.hpp"

miniredis::server::server (const std::string& address, const std::string& port) : 
  io_context_ (),
  signals_ (io_context_),
  acceptor_ (io_context_),
  manager_ ()
{
  signals_.add (SIGTERM);
  signals_.add (SIGINT);

  await_stop ();

  boost::asio::ip::tcp::resolver resolver (io_context_);
  boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve (address, port).begin();
  
  acceptor_.open (endpoint.protocol ());
  acceptor_.set_option (boost::asio::ip::tcp::acceptor::reuse_address (true));
  acceptor_.bind (endpoint);
  acceptor_.listen ();

  start_accept ();
}

void miniredis::server::run ()
{
  /*
  instead of io_context.run() we are going to use .poll () instead
  */

 while (!closed_)
 {
  if (io_context_.stopped ())
  {
    io_context_.restart ();
  }

  if (!io_context_.poll ())
  {
    std::cout << "tasks need to be implemented";
  }

  else
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
  }
 }

}

void miniredis::server::start_accept ()
{
  std::cout << "success" << std::flush;

  acceptor_.async_accept (
    [this] (boost::asio::ip::tcp::socket socket, boost::system::error_code error)
  {
    if (!acceptor_.is_open ())
    {
      return;
    }

    if (!error)
    {
      manager_.start (std::make_shared <connection> (std::move (socket), manager_));
    }

    start_accept ();
    
  });
}

void miniredis::server::await_stop ()
{
  signals_.async_wait (
    [this] (boost::system::error_code, int)
  {
    acceptor_.close ();
    manager_.stop_all ();
    closed_ = true;
  });
}