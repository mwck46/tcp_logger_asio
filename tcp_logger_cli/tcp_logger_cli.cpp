/*****************************************************
This project is built with visual studio 2017 build-in 
c++ compiler (version v141), which only support 
*****************************************************/

#include <cstdio>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <boost/bind.hpp>

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
      std::istream is(&_streamBufr);
      std::string line;
      std::getline(is, line);

      boost::posix_time::ptime timeLocal = boost::posix_time::second_clock::local_time();
      std::string clientIp = this->socket_.remote_endpoint().address().to_string();
      std::printf("[%s] %s: %s\r\n", to_simple_string(timeLocal).c_str(), clientIp.c_str(), line.c_str());
    }
  }

  void do_read()
  {
    auto self(shared_from_this());

    // Points to note:
    // 1) When you call an asynchronous read or write, you need to ensure that the buffers for the operation are valid until the completion handler is called
    //  i.e. don't use local variable
    

    // async_read_some() 
    //    call handler immediately after receive 'some' data. In debug mode, it can read all received data, but in release mode, it can only read ~1 byte
    //socket_.async_read_some(boost::asio::buffer(data_, max_length), boost::bind(&session::handler, shared_from_this(), _1, _2));

    // async_read()
    //async_read(socket_, boost::asio::buffer(data_, max_length), boost::asio::transfer_at_least(max_length), boost::bind(&session::handler, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    
    // async_read_until() 
    //    call handler until delimiter is reached. If delimiter is not reached, it still calls handler when connection is closed
    async_read_until(socket_, _streamBufr, "\n", boost::bind(&session::async_read_until_handler, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
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

  boost::asio::streambuf _streamBufr;
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
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

int main(int argc, char* argv[])
{
  try
  {
    int port = 50001;
    if (argc == 2)
    {
      port = std::atoi(argv[1]);
    }
    else
    {
      //std::cerr << "Usage: async_tcp_echo_server <port>\n";
      //return 1;
    }

    boost::asio::io_context io_context;

    server s(io_context, port);

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
