//
// Created by rmq on 5/23/24.
//
#ifndef CHAT_MESSAGE_H
#define CHAT_MESSAGE_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <algorithm>
#include <optional>
#include "Common.h"
#include "spdlog/spdlog.h"

struct __attribute__ ((__packed__))  message_header {
    uint64_t msg_length = 0;
    uint64_t msg_type;
    uint64_t msg_src;
    uint64_t msg_target = 0;
    uint64_t msg_seq;

    std::string to_string() const {
        return "len:" + std::to_string(msg_length) + " type:" + std::to_string(msg_type) + " src:" +
               std::to_string(msg_src) + " target: " + std::to_string(msg_target) + " seq:" +
               std::to_string(msg_seq);
    }
};


class ChatMessage {
public:
    static constexpr size_t header_length = sizeof(message_header) / sizeof(char);
    static constexpr size_t size_of_uint64 = sizeof(uint64_t) / sizeof(char);
    static constexpr size_t max_body_length = 8192;

    static constexpr uint64_t msg_type_new = 65UL;
    static constexpr uint64_t msg_type_ack = 66UL;


    ChatMessage(uint64_t msg_type, uint64_t msg_src, uint64_t msg_seq) : data_(new std::vector<char>(header_length)) {
        header_.msg_type = msg_type;
        header_.msg_src = msg_src;
        header_.msg_seq = msg_seq;
    }

    ChatMessage(uint64_t msg_type, uint64_t msg_src, uint64_t msg_seq, uint64_t body_length_) : ChatMessage(msg_type,
                                                                                                              msg_src,
                                                                                                              msg_seq) {
        reserve_body(body_length_);
    }

    ChatMessage() = delete;

    void release_body() {
        data_ = std::make_shared<std::vector<char>>(header_length);
    }

    const char *data() const {
        return data_->data();
    }

    char *data() {
        return data_->data();
    }

    std::size_t length() const {
        return data_->size();
    }

    const message_header &header() const {
        return header_;
    }

    message_header &header() {
        return header_;
    }

    const char *body() const {
        return data() + header_length;
    }

    char *body() {
        return data() + header_length;
    }

    std::size_t body_length() const {
        return header_.msg_length - header_length;
    }

    void reserve_body(std::size_t new_body_length) {
        data_->resize(new_body_length + header_length);
        header_.msg_length = data_->size();
    }

    void reserve(std::size_t new_length) {
        data_->resize(new_length);
        header_.msg_length = data_->size();
    }

    bool decode_header() {
        if (data_->size() < header_length) {
            return false;
        }
        const char *buf = data();
        std::memcpy(reinterpret_cast<void *>(&header_), reinterpret_cast<const void *>(buf), header_length);

        if (header_.msg_type == msg_type_new) {
            return true;
        } else if (header_.msg_type == msg_type_ack) {
            if (header_.msg_length != header_length + size_of_uint64) {
//                throw std::invalid_argument("cannot parse ack msg");
                spdlog::warn("cannot parse ack msg");
                return false;
            }
            return true;
        } else {
//            throw std::invalid_argument("unknown msg type");
            spdlog::warn("cannot parse unknown msg with type {}", header_.msg_type);
            return false;
        }
        return false;
    }

    void encode_header() {
        char *buf = data();
        memcpy(reinterpret_cast<void *>(buf), reinterpret_cast<const void *>(&header_), header_length);
    }

private:
    std::shared_ptr<std::vector<char>> data_;
    message_header header_;
};


class message_tracker {
public:
    static constexpr size_t max_tracking_message = 8192;

    void send(uint64_t seq) {
        send_time_[seq] = time_since_epoch_micro();
        if (send_time_.size() > max_tracking_message) {
            send_time_.erase(send_time_.begin(),
                             std::next(send_time_.begin(),
                                       static_cast<int64_t>(send_time_.size() - max_tracking_message)));
        }
    }

    std::optional<uint64_t> ack(uint64_t seq) {
        auto it = send_time_.find(seq);
        if (it != send_time_.end()) {
            auto rtt = time_since_epoch_micro() - it->second;
            rtt_records_[seq] = rtt;
            return rtt;
        }
        return std::nullopt;
    }


private:
    std::map<uint64_t, int64_t> send_time_; // message seq to send_time, used for calulating rtt.
    std::map<uint64_t, int64_t> rtt_records_; // message seq to rtt time in nanoseconds.
};

#endif // CHAT_MESSAGE_H
