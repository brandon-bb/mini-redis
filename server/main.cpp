#include "server.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <string>

int main ()
{
  try
  {
    miniredis::server s ("127.0.0.1", "3945");
    s.run ();
  }
  
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }

  return 0;
  
}

/*
* issue 1: segmentation fault (core dumped) - resolved by switching the declaration
  order of io_context and accpetor in server.hpp
*/


