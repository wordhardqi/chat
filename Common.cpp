#include <atomic>
#include "Common.h"

int64_t time_since_epoch_micro() {
    using namespace std::chrono;
    return static_cast<int64_t>( duration_cast<microseconds>(system_clock::now().time_since_epoch()).count());
}

uint64_t generate_seq() {
    static std::atomic<uint64_t> global_seq{0};
    return global_seq.fetch_add(1UL);
}

void serial_uint64(const uint64_t src, char *buf) {
    auto *src_addr = reinterpret_cast<const void *>(&src);
    memcpy(reinterpret_cast<void *>(buf), src_addr, sizeof(uint64_t) / sizeof(char));
}

void parse_uint64(const char *buf, uint64_t &target) {
    auto *dst_addr = reinterpret_cast<void *>(&target);
    memcpy(dst_addr, reinterpret_cast<const void *>(buf), sizeof(uint64_t) / sizeof(char));
}

