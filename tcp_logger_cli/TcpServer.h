#include "TcpSession.h"

using boost::asio::ip::tcp;

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