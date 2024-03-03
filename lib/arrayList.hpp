#ifndef ARRAYLIST_HPP_
#define ARRAYLIST_HPP_
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <vector>
#include <condition_variable>

template<class T>
struct ArrayListNode{
    T value;
    u_int32_t next;
};

template<class T>
class ArrayList{
    mutable std::shared_mutex mutex_;
    const u_int32_t maxLength;
    const u_int32_t warningLength; //once exceed warningLength, warning = true
    std::atomic_bool warning;
    std::atomic_uint32_t nodeNum; // now number of nodes, only increase

    std::atomic_uint32_t threadCount;
    std::condition_variable cv;

    u_int8_t* idArray;
    ArrayListNode<T>* array;
public:
    ArrayList(u_int32_t maxLength, u_int32_t warningLength):maxLength(maxLength),warningLength(warningLength){
        this->array = new ArrayListNode<T>[maxLength];
        this->idArray = new u_int8_t[maxLength];
        this->nodeNum = 0;
        this->warning = false;
        this->threadCount = 0;
    }
    ~ArrayList(){
        if(this->threadCount){
            std::cout << "Array list warning: it is used by certain thread." <<std::endl;
        }
        delete[] array;
        delete[] idArray;
    }
    // add node to the tail, and return the tail, make sure only one thread can use this function
    u_int32_t addNodeOneThread(T value, u_int8_t id){
        if(this->nodeNum >= this->maxLength){
            std::cerr << "Array list error: addNodeOneThread overflow the buffer!" <<std::endl;
            return this->nodeNum;
        }
        u_int32_t now_num = this->nodeNum++;
        this->array[now_num].value = value;
        this->idArray[now_num] = id;
        if(now_num >= this->warningLength){
            this->warning = true;
        }
        this->cv.notify_all();
        return now_num;
    }
    //change next of pos, make sure only one thread use it or each thread change in different pos.
    bool changeNextOneThread(u_int32_t pos, u_int32_t next){
        if(pos > this->maxLength || pos < 0){
            std::cerr << "Array list error: changeNext overflow the buffer!" <<std::endl;
            return false;
        }
        this->array[pos].next = next;
        return true;
    }
    //change next of pos
    bool changeNextMultiThread(u_int32_t pos, u_int32_t next){
        if(pos > this->maxLength || pos < 0){
            std::cerr << "Array list error: changeNext overflow the buffer!" <<std::endl;
            return false;
        }

        //std::unique_lock<std::shared_mutex> lock(mutex_);
        this->array[pos].next = next;
        lock.unlock()

        return true;
    }
    void addThread(){
        this->threadCount++;
    }
    void ereaseThread(){
        this->threadCount--;
    }
    bool getWarning()const{
        return this->warning;
    }
    u_int32_t getNodeNum()const{
        return this->nodeNum;
    }
    u_int32_t getLength()const{
        return this->nodeNum*sizeof(ArrayListNode<T>);
    }
    // Parallelizable and read only, get ID from certain pos
    u_int8_t getID(u_int32_t pos){
        if(pos > this->maxLength || pos < 0){
            std::cerr << "Array list error: getID overflow the buffer!" <<std::endl;
            return T();
        }

        std::shared_lock<std::shared_mutex> lock(mutex_);
        this->cv.wait(lock, [] { return pos < this->nodeNum; });

        return this->idArray;
    }
    // Parallelizable and read only, get value from certain pos, each read thread read on certain id
    T getValue(u_int32_t pos, u_int8_t id){
        if(pos > this->maxLength || pos < 0){
            std::cerr << "Array list error: getValue overflow the buffer!" <<std::endl;
            return T();
        }
        if(pos >= this->nodeNum){
            std::cerr << "Array list error: getValue overflow the node count!" <<std::endl;
            return T();
        }
        if(id != this->idArray[pos]){
            std::cerr << "Array list error: getValue on wrong ID!" <<std::endl;
            return T();
        }
        // std::unique_lock<std::shared_mutex> lock(mutex_);
        return this->array[pos].value;
    }
    // Parallelizable
    u_int32_t getNext(u_int32_t pos){
        if(pos > this->maxLength || pos < 0){
            std::cerr << "Array list error: getNext overflow the buffer!" <<std::endl;
            return 0;
        }
        if(pos >= this->nodeNum){
            std::cerr << "Array list error: getNext overflow the node count!" <<std::endl;
            return T();
        }
        // std::unique_lock<std::shared_mutex> lock(mutex_);
        return this->array[pos].next;
    }
    char* outputToChar()const{
        if(this->threadCount){
            std::cerr << "Array list error: outputToChar while it is used by certain thread!" <<std::endl;
            return nullptr;
        }
        u_int32_t len = this->getLength();
        char* output = new char[len];
        memcpy(output,this->array,len);
        return output;
    }
};


#endif