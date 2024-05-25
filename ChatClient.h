//
// Created by rmq on 5/25/24.
//

#ifndef CHAT_CHATCLIENT_H
#define CHAT_CHATCLIENT_H

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <unordered_map>
#include "ChatMessage.h"
#include "Common.h"
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

using boost::asio::ip::tcp;

typedef std::deque<ChatMessage> chat_message_queue;


class ChatClient {
public:
    static constexpr size_t server_client_id = 65;
    ChatClient(boost::asio::io_context &io_context,
               const tcp::resolver::results_type &endpoints, size_t id);

    void write(const ChatMessage &msg);

    void close() {
        boost::asio::post(io_context_, [this]() { socket_.close(); });
    }

    size_t get_id() {
        return client_id;
    }

private:
    void do_connect(const tcp::resolver::results_type &endpoints);

    // read side.

    void do_read_header();

    void do_read_body();

    static void print_msg(const uint64_t id, const uint64_t msg_seq, const std::string &str);

    bool handle_msg();

    // write side.
    void do_write();


private:
    uint64_t client_id = 0UL;
    boost::asio::io_context &io_context_;
    tcp::socket socket_;
    ChatMessage read_msg_;
    chat_message_queue write_msgs_;
    MessageTracker msgs_tracker_;
};

#endif //CHAT_CHATCLIENT_H
