#include "connection.hpp"

using namespace boost::asio::ip;


miniredis::connection::connection (tcp::socket socket, connection_manager& manager) : 
socket_(std::move (socket)), connection_manager_ (manager)
{
}

void miniredis::connection::start ()
{
 async_write ();
}

void miniredis::connection::stop ()
{
  socket_.close ();
}

void miniredis::connection::async_write ()
{
  message_ = "pppo";
  auto self(shared_from_this ());

  boost::asio::async_write (socket_, boost::asio::buffer (message_),
  [this, self](boost::system::error_code error, std::size_t)
  {
    if (!error)
    {
      boost::system::error_code ignored_error;
      socket_.shutdown (tcp::socket::shutdown_both, ignored_error);
    }

    if (error != boost::asio::error::operation_aborted)
    {
      connection_manager_.stop (shared_from_this ());
    }
  });
}

miniredis::connection_manager::connection_manager ()
{
}

void miniredis::connection_manager::start (connection::connection_pointer c)
{
  connections_.insert (c);
  c->start ();
}

void miniredis::connection_manager::stop (connection::connection_pointer c)
{
  connections_.erase (c);
  c->stop ();
}

void miniredis::connection_manager::stop_all ()
{
  for (auto c : connections_)
  {
    c->stop ();
  }

  connections_.clear ();
}
