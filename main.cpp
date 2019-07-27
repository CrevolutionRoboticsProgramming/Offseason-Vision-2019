//
// main.cpp
// ~~~~~~~~
//
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "http/server/server.hpp"

std::string address{"localhost"};
std::string port{"8080"};
std::string doc_root{"."};

int main()
{
  try
  {
    // Initialise the server.
    http::server::server server(address, port, doc_root);

    // Run the server until stopped.
    server.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}