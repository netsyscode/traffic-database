#include "truncateChecker.hpp"

void TruncateChecker::run(){
    printf("TruncateChecker log: thread run.\n");
    this->stop = false;
    while (!this->stop){
        for(auto t:*(this->truncators)){
            t->check();
            t->update();
        }
    }
}

void TruncateChecker::asynchronousStop(){
    this->stop = true;
}