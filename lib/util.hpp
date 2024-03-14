#ifndef UTIL_HPP_
#define UTIL_HPP_
#include <cstdlib>

struct ThreadPointer{
    u_int32_t id;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic_bool stop_;
    std::atomic_bool pause_;
};

struct Index{
    std::string key;
    u_int32_t value;
};

#endif