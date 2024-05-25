//
// Created by rmq on 5/25/24.
//
#include "ChatClient.h"
#include <boost/asio/error.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/read_until.hpp>
#include <cstdint>

void async_read_input(boost::asio::io_context &io_context, ChatClient &c,
                      boost::asio::streambuf &input_buf,
                      boost::asio::posix::stream_descriptor &input_stream) {
  boost::asio::async_read_until(
      input_stream, input_buf, '\n',
      [&](boost::system::error_code ec, std::size_t bytes_transferred) {
        if (!ec) { // got newline
          ChatMessage msg(ChatMessage::msg_type_new, c.get_id(), generate_seq(),
                          bytes_transferred - 1);
          input_buf.sgetn(msg.body(),
                          static_cast<int64_t>(bytes_transferred - 1));
          input_buf.consume(1);
          c.write(msg);
          async_read_input(io_context, c, input_buf, input_stream);
        } else if (ec == boost::asio::error::not_found ||
                   (ec == boost::asio::error::eof &&
                    bytes_transferred > 0)) { // no newline
          ChatMessage msg(ChatMessage::msg_type_new, c.get_id(), generate_seq(),
                          bytes_transferred);
          input_buf.sgetn(msg.body(), static_cast<int64_t>(bytes_transferred));
          c.write(msg);
          async_read_input(io_context, c, input_buf, input_stream);
        } else {
          spdlog::info("input read error {}", ec.message());
          io_context.stop();
        }
      });
}

int main(int argc, char *argv[]) {
  spdlog::cfg::load_env_levels();
  spdlog::info("chat client start");
  try {
    if (argc != 4) {
      std::cerr << "Usage: ChatClient <id> <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    size_t id = static_cast<size_t>(std::atoll(argv[1]));
    if (id == ChatClient::server_client_id) {
      std::cerr << "id 65 is reserved for server, use another id";
      return 1;
    }
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[2], argv[3]);
    ChatClient c(io_context, endpoints, id);
    // std::thread t([&io_context]() { io_context.run(); });

    boost::asio::posix::stream_descriptor input_stream(io_context,
                                                       STDIN_FILENO);
    boost::asio::streambuf input_buf;
    input_buf.prepare(ChatMessage::max_body_length + 1);
    async_read_input(io_context, c, input_buf, input_stream);

    io_context.run();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
