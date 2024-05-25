//
// Created by rmq on 5/23/24.
//
#include "ChatClient.h"


void ChatClient::do_write() {
    auto &front = write_msgs_.front();
    auto current_seq = front.header().msg_seq;
    msgs_tracker_.send(current_seq);
    spdlog::debug("write {}", front.header().to_string());
    front.encode_header();
    boost::asio::async_write(socket_,
                             boost::asio::buffer(write_msgs_.front().data(),
                                                 write_msgs_.front().length()),
                             [this](boost::system::error_code ec, std::size_t /*length*/) {
                                 if (!ec) {
                                     write_msgs_.pop_front();
                                     if (!write_msgs_.empty()) {
                                         do_write();
                                     }
                                 } else {
                                     socket_.close();
                                 }
                             });
}

ChatClient::ChatClient(boost::asio::io_context &io_context,
                       const boost::asio::ip::basic_resolver<tcp, boost::asio::any_io_executor>::results_type &endpoints,
                       size_t id)
        : client_id(id), io_context_(io_context),
          socket_(io_context), read_msg_(ChatMessage::header_length, 0, 0) {
    do_connect(endpoints);
    socket_.set_option(tcp::no_delay(true));
}

void ChatClient::write(const ChatMessage &msg) {
    assert(msg.header().msg_length != 0);
    boost::asio::post(io_context_,
                      [this, msg]() {
                          bool write_in_progress = !write_msgs_.empty();
                          write_msgs_.push_back(msg);
                          if (!write_in_progress) {
                              do_write();
                          }
                      });
}

void ChatClient::do_connect(
        const boost::asio::ip::basic_resolver<tcp, boost::asio::any_io_executor>::results_type &endpoints) {
    boost::asio::async_connect(socket_, endpoints,
                               [this](boost::system::error_code ec, tcp::endpoint) {
                                   if (!ec) {
                                       do_read_header();
                                   }
                               });
}

void ChatClient::do_read_header() {
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.data(), ChatMessage::header_length),
                            [this](boost::system::error_code ec, std::size_t /*length*/) {
                                if (!ec && read_msg_.decode_header()) {
                                    do_read_body();
                                } else {
                                    socket_.close();
                                }
                            });
}

bool ChatClient::handle_msg() {
    spdlog::debug("recv {}", read_msg_.header().to_string());
    switch ((read_msg_.header().msg_type)) {
        case ChatMessage::msg_type_new: {
            print_msg(read_msg_.header().msg_src, read_msg_.header().msg_seq,
                      std::string(read_msg_.body(), read_msg_.body_length()));
            if (read_msg_.header().msg_src != get_id()) {
                ChatMessage ack_msg(ChatMessage::msg_type_ack, client_id, generate_seq());
                ack_msg.reserve_body(ChatMessage::size_of_uint64);
                ack_msg.header().msg_target = read_msg_.header().msg_src;
                serial_uint64(read_msg_.header().msg_seq, ack_msg.body());
                write(ack_msg);
            }
            return true;
        }
        case ChatMessage::msg_type_ack: {
            uint64_t src_seq;
            parse_uint64(read_msg_.body(), src_seq);
            auto rtt_opt = msgs_tracker_.ack(src_seq);
            if (rtt_opt.has_value()) {
                spdlog::info("acked {}, rtt {}us", src_seq, rtt_opt.value());
            }
            return true;
        }
        default: {
            spdlog::warn(" unknown msg {}", read_msg_.header().to_string());
            return true;
        }
    }
}

void ChatClient::print_msg(const uint64_t id, const uint64_t seq, const std::string &str) {
    spdlog::info("\x1b[31m client-{} msg-{}:  {} \x1b[0m", id, seq, str);
}

void ChatClient::do_read_body() {
    read_msg_.reserve(read_msg_.header().msg_length);
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                            [this](boost::system::error_code ec, std::size_t /*length*/) {
                                if (!ec) {
                                    bool can_handle = handle_msg();
                                    if (can_handle) {
                                        do_read_header();
                                        return;
                                    }
                                }
                                // err path.
                                socket_.close();

                            });
}
