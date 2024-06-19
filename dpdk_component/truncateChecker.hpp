#ifndef TRUNCATECHECKER_HPP_
#define TRUNCATECHECKER_HPP_
#include <iostream>
#include "../dpdk_lib/truncator.hpp"

class TruncateChecker{
private:
    std::vector<Truncator*>* truncators;
    bool stop;
public:
    TruncateChecker(std::vector<Truncator*>* truncators){
        this->truncators = truncators;
        this->stop = true;
    }
    ~TruncateChecker() = default;
    void run();
    void asynchronousStop();
};

#endif