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

#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <boost/bind.hpp>

#include <boost/thread.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/chrono.hpp>


using boost::asio::ip::tcp;


class session : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket) : socket_(std::move(socket))
  {
    memset(data_, 0, max_length);
  }

  void start()
  {
    do_read();
  }

private:

  void handler(const boost::system::error_code& ec, std::size_t size)
  {
    if (!ec)
    {
      boost::posix_time::ptime timeLocal = boost::posix_time::second_clock::local_time();
      std::string clientIp = this->socket_.remote_endpoint().address().to_string();
      data_[29] = '\0';
      std::printf("[%s] %s: %s\r\n", to_simple_string(timeLocal).c_str(), clientIp.c_str(), data_);
      memset(data_, 0, max_length);
    }
  }

  void async_read_until_handler(const boost::system::error_code& ec, std::size_t size)
  {
    if (!ec)
    {
      // record receiving time
      boost::posix_time::ptime timeLocal = boost::posix_time::second_clock::local_time();
      std::string clientIp = this->socket_.remote_endpoint().address().to_string();

      snprintf(_logMsg, sizeof(_logMsg), "[%s] %s: %s", to_simple_string(timeLocal).c_str(), clientIp.c_str(), dynBuf.c_str());

      unsigned short p = socket_.local_endpoint().port();
      char filename[256];
      snprintf(filename, sizeof(filename), "port_%hu.log", p);
      FILE * ofile = fopen(filename, "a");
      if (!ofile)
      {
        std::printf("Cannot open log file");
        return;
      }
      //std::printf("%s\n", _logMsg);
      std::fprintf(ofile, "%s", _logMsg);
      fclose(ofile);
    }
  }

  void do_read()
  {
    auto self(shared_from_this());

    //
    // When you call an asynchronous read or write, you need to ensure that the buffers for the operation are valid until the completion handler is called
    //    i.e. don't use local variable
    //

    // async_read_some() 
    //    call handler immediately after receive 'some' data. In debug mode, it can read all received data, but in release mode, it can only read ~1 byte
    //socket_.async_read_some(boost::asio::buffer(data_, max_length), boost::bind(&session::handler, shared_from_this(), _1, _2));

    // async_read()
    //async_read(socket_, boost::asio::buffer(data_, max_length), boost::asio::transfer_at_least(max_length), boost::bind(&session::handler, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    
    // async_read_until() 
    //    call handler until delimiter is reached. If delimiter is not reached, it still calls handler when connection is closed
    //
    // 1) Use stream buffer
    //async_read_until(socket_, _streamBufr, "\n", boost::bind(&session::async_read_until_handler, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    //
    // 2) Use dynamic buffer (cleaner)
    async_read_until(socket_, boost::asio::dynamic_buffer(dynBuf), "\n", boost::bind(&session::async_read_until_handler, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length), [this, self](boost::system::error_code ec, std::size_t length)
    {
      if (!ec)
      {
        do_read();
      }
    });
  }

  tcp::socket socket_;
  enum { max_length = 30 };
  char data_[max_length];
  char _logMsg[100];

  boost::asio::streambuf _streamBufr;
  std::string dynBuf;
};
class server
{
public:
  server(boost::asio::io_context& io_context, short port) : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    char filename[256];
    snprintf(filename, sizeof(filename), "port_%hu.log", port);
    std::printf("Listening to port %hu, log file: %s\n", port, filename);
    do_accept();
  }


private:
  void do_accept()
  {
    acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket)
    {
      if (!ec)
      {
        std::make_shared<session>(std::move(socket))->start();
      }

      do_accept();
    });
  }

  tcp::acceptor acceptor_;
};

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


/*
class session
{
public:
  session(boost::asio::io_service& io_service): socket_(io_service)
  {
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    std::cout << "starting" << std::endl;
    boost::asio::async_read_until(socket_, _streamBufr, ' ',
      boost::bind(&session::handle_read, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
  }

  void handle_read(const boost::system::error_code& error, size_t bytes_transferred)
  {
    std::cout << "handling read" << std::endl;
    if (!error)
    {
      boost::asio::async_write(socket_,
        boost::asio::buffer(data_, bytes_transferred),
        boost::bind(&session::handle_write, this,
          boost::asio::placeholders::error));
    }
    else
    {
      delete this;
    }
  }

  void handle_write(const boost::system::error_code& error)
  {
    if (!error)
    {
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            boost::bind(&session::handle_read, this,
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      delete this;
    }
  }

private:
  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
  boost::asio::streambuf _streamBufr;
};
class server
{
public:
  server(boost::asio::io_service& io_service, short port)
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
  {
    session* new_session = new session(io_service_);
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

  void handle_accept(session* new_session,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      new_session->start();
      new_session = new session(io_service_);
      acceptor_.async_accept(new_session->socket(),
          boost::bind(&server::handle_accept, this, new_session,
            boost::asio::placeholders::error));
    }
    else
    {
      delete new_session;
    }
  }

private:
  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
  try
  {
    boost::asio::io_service io_service;

    using namespace std; // For atoi.
    server s(io_service, 50001);

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

*/
