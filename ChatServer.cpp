#include "ChatServer.h"

void ChatRoom::join(ChatParticipantPtr participant) {
    spdlog::debug("participant joined");
    participants_.insert(participant);
    for (auto msg: recent_msgs_)
        participant->deliver(msg);
}

void ChatRoom::leave(ChatParticipantPtr participant) {
    spdlog::debug("participant left");
    participants_.erase(participant);

}

void ChatRoom::deliver(const ChatMessage &msg) {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
        recent_msgs_.pop_front();
    auto msg_type = msg.header().msg_type;
    if (msg_type == msg.msg_type_ack) {
        for (auto participant: participants_) {
            if (participant->get_id() == msg.header().msg_target) {
                participant->deliver(msg);
            }
        }
    } else if (msg_type == msg.msg_type_new) {
        for (auto participant: participants_) {
            participant->deliver(msg);
        }
    }

}

ChatSession::ChatSession(tcp::socket socket, ChatRoom &room)
        : socket_(std::move(socket)),
          room_(room), read_msg_(ChatMessage::header_length, 0, 0) {
}

void ChatSession::start() {
    room_.join(shared_from_this());
    do_read_header();
}

void ChatSession::deliver(const ChatMessage &msg) {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
        do_write();
    }
}

void ChatSession::do_read_header() {
    read_msg_.release_body();
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.data(), ChatMessage::header_length),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                                if (!ec && read_msg_.decode_header()) {
                                    set_id(read_msg_.header().msg_src);
                                    if (read_msg_.body_length() > 0) {
                                        do_read_body();
                                    } else {
                                        do_read_header();
                                    }
                                } else {
                                    room_.leave(shared_from_this());
                                }
                            });
}

void ChatSession::do_read_body() {
    read_msg_.reserve(read_msg_.header().msg_length);
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
                            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                                if (!ec) {
                                    spdlog::debug("recv {}", read_msg_.header().to_string());
                                    room_.deliver(read_msg_);
                                    do_read_header();
                                } else {
                                    room_.leave(shared_from_this());
                                }
                            });
}

void ChatSession::do_write() {
    auto &front = write_msgs_.front();
    front.encode_header();
    spdlog::debug("send {} msg: {} {}", front.header().to_string(), front.body_length(),
                  std::string(front.body(), front.body_length()));
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
                             boost::asio::buffer(front.data(),
                                                 front.length()),
                             [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                                 if (!ec) {
                                     write_msgs_.pop_front();
                                     if (!write_msgs_.empty()) {
                                         do_write();
                                     }
                                 } else {
                                     room_.leave(shared_from_this());
                                 }
                             });
}

void ChatSession::set_id(const size_t id) {
    session_id = id;
}

size_t ChatSession::get_id() {
    return session_id;
}

ChatServer::ChatServer(boost::asio::io_context &io_context, const tcp::endpoint &endpoint)
        : acceptor_(io_context, endpoint) {
    do_accept();
}

void ChatServer::do_accept() {
    acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    socket.set_option(tcp::no_delay(true));
                    std::make_shared<ChatSession>(std::move(socket), room_)->start();
                }

                do_accept();
            });
}
