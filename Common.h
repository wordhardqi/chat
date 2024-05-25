#ifndef CHAT_COMMON_H
#define CHAT_COMMON_H

#include <chrono>
#include <spdlog/spdlog.h>

int64_t time_since_epoch_micro();
uint64_t generate_seq();

void serial_uint64(const uint64_t src, char *buf) ;
void parse_uint64(const char *buf, uint64_t &target);

#endif //CHAT_COMMON_H
