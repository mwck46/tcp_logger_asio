/*****************************************************
Point of interest:
1) async_read_some(), async_read(), async_read_until()
2) boost::lockfree::queue
*****************************************************/
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS //fopen
#endif

#include <cstdio>
#include <iostream>
#include <memory>
#include <utility>
#include <string>

#include"TcpServer.h"
#include <boost/asio.hpp>

void worker(int port) {
  boost::asio::io_context io_context;
  server s(io_context, (short)port);
  io_context.run();
}

int main(int argc, char* argv[])
{
  try
  {
    const int numPort = 5;
    const int ports[numPort] = { 40001, 41001, 40002, 41002, 50001 };
    boost::thread *threads[numPort];

    // Creation
    for(int i = 0; i < numPort; i++) {
      threads[i] = new boost::thread(worker, ports[i]);
    }

    // Cleanup
    for(int i = 0; i < numPort; i++) {
        threads[i]->join();
        delete threads[i];
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}