//
// Created by rmq on 5/25/24.
//

#ifndef CHAT_CHATSERVER_H
#define CHAT_CHATSERVER_H


#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "ChatMessage.h"
#include "Common.h"
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

using boost::asio::ip::tcp;

typedef std::deque<ChatMessage> ChatMessageQueue;

class ChatParticipant {
public:
    virtual ~ChatParticipant() {}

    virtual void deliver(const ChatMessage &msg) = 0;

    virtual void set_id(const size_t id) = 0;

    virtual size_t  get_id() =0;

    virtual void close() = 0 ;
};

typedef std::shared_ptr<ChatParticipant> ChatParticipantPtr;


class ChatRoom {
public:
    void join(ChatParticipantPtr participant);

    void leave(ChatParticipantPtr participant);

    void deliver(const ChatMessage &msg);

    void close();

    size_t num_participants() {return participants_.size();}
private:
    std::set<ChatParticipantPtr> participants_;
};


class ChatSession
        : public ChatParticipant,
          public std::enable_shared_from_this<ChatSession> {
public:
    ChatSession(tcp::socket socket, ChatRoom &room);

    void start();

    void deliver(const ChatMessage &msg);

    void set_id(const size_t id);

    size_t get_id();

    void close();

private:
    void do_read_header();

    void do_read_body();

    void do_write();

    tcp::socket socket_;
    ChatRoom &room_;
    ChatMessage read_msg_;
    ChatMessageQueue write_msgs_;
    size_t session_id = 0 ;
};


class ChatServer {
public:
    ChatServer(boost::asio::io_context &io_context,
               const tcp::endpoint &endpoint);

    void close();
private:
    void do_accept();
    tcp::acceptor acceptor_;
    ChatRoom room_;
};


#endif //CHAT_CHATSERVER_H
