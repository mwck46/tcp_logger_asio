#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <boost/bind.hpp>

#include <boost/thread.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/chrono.hpp>

#include <fstream>

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
      std::fstream fsLog;
      fsLog.open(filename, std::fstream::out | std::fstream::app);
      if (fsLog)
      {
        fsLog << _logMsg << std::endl;
        fsLog.close();
      }
      else
      {
        std::cout << "[Error] Cannot open log file: " << filename << std::endl;
      }
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
