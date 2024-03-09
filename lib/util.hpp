#ifndef UTIL_HPP_
#define UTIL_HPP_
#include <cstdlib>

struct ThreadReadPointer{
    u_int32_t id;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic_bool stop_;
};

#endif