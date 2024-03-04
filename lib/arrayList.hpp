#ifndef ARRAYLIST_HPP_
#define ARRAYLIST_HPP_
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <vector>
#include <condition_variable>

struct ThreadReadPointer{
    u_int32_t id;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic_bool stop_;
};

struct IDData{
    u_int8_t data;
    u_int8_t err;
};

template<class T>
struct ArrayListNode{
    T value;
    u_int32_t next;
};

template<class T>
class ArrayList{
    const u_int32_t maxLength;
    const u_int32_t warningLength; //once exceed warningLength, warning = true
    std::atomic_bool warning;
    std::atomic_uint32_t nodeNum; // now number of nodes, only increase

    std::atomic_uint32_t threadCount; // read + write thread count
    std::vector<ThreadReadPointer*> writeThreads;

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
            return std::numeric_limits<uint32_t>::max();
        }
        u_int32_t now_num = this->nodeNum++;
        this->array[now_num].value = value;
        this->array[now_num].next = std::numeric_limits<uint32_t>::max();
        this->idArray[now_num] = id;
        if(now_num >= this->warningLength){
            this->warning = true;
        }
        for(auto t:this->writeThreads){
            t->cv_.notify_one();
        }
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
        this->array[pos].next = next;
        return true;
    }
    //only controller can modify threads
    void addWriteThread(){
        this->threadCount++;
    }
    void ereaseWriteThread(){
        this->threadCount--;
    }
    bool addReadThread(ThreadReadPointer* thread){
        for(auto t:this->writeThreads){
            if(t->id == thread->id){
                std::cerr << "Array list error: addReadThread with exist id!" <<std::endl;
                return false;
            }
        }
        thread->stop_ = false;
        this->writeThreads.push_back(thread);
        this->threadCount++;
        return true;
    }
    bool ereaseReadThread(ThreadReadPointer* thread){
        for(auto it = this->writeThreads.begin();it!=this->writeThreads.end();++it){
            if((*(it))->id == thread->id){
                this->writeThreads.erase(it);
                this->threadCount--;
                return true;
            }
        }
        std::cerr << "Array list error: ereaseReadThread with non-exist id!" <<std::endl;
        return false;
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
    IDData getID(u_int32_t pos, u_int32_t thread_id){
        IDData data = {
            .data = 0,
            .err = 0,
        };
        if(pos > this->maxLength || pos < 0){
            std::cerr << "Array list error: getID overflow the buffer!" <<std::endl;
            data.err = 1;
            return data;
        }

        ThreadReadPointer* thread = nullptr;
        for(auto t:this->writeThreads){
            if(t->id == thread->id){
                thread = t;
            }
        }
        if(thread == nullptr){
            std::cerr << "Array list error: getID with non-exist thread id!" <<std::endl;
            data.err = 2;
            return data;
        }

        std::unique_lock<std::mutex> lock(thread->mutex_);
        thread->cv_.wait(lock, [&pos,this,thread] { return pos < this->nodeNum || thread->stop_; });
        
        if(thread->stop_){
            std::cout << "Array list log: thread" << thread_id << "stop." <<std::endl;
            data.err = 3;
            return data;
        }

        data.data = this->idArray[pos];
        return data;
    }
    // Non-parallelizable, get ID from certain pos
    IDData getIDOneThread(u_int32_t pos){
        IDData data = {
            .data = 0,
            .err = 0,
        };
        if(pos > this->maxLength || pos < 0){
            std::cerr << "Array list error: getID overflow the buffer!" <<std::endl;
            data.err = 1;
            return data;
        }
        if(pos > this->nodeNum){
            std::cerr << "Array list error: getID overflow the node number!" <<std::endl;
            data.err = 2;
            return data;
        }
        if(this->threadCount){
            std::cerr << "Array list error: outputToChar while it is used by certain thread!" <<std::endl;
            data.err = 3;
            return data;
        }
        
        data.data = this->idArray[pos];
        return data;
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
    CharData outputToChar()const{
        CharData data = {
            .data = nullptr,
            .len = 0,
        };
        if(this->threadCount){
            std::cerr << "Array list error: outputToChar while it is used by certain thread!" <<std::endl;
            return data;
        }
        data.len = this->getLength();
        data.data = new char[data.len];
        memcpy(data.data,this->array,data.len);
        return data;
    }
    void asynchronousStop(u_int32_t threadID){
        for(auto it = this->writeThreads.begin();it!=this->writeThreads.end();++it){
            if((*(it))->id == threadID){
                (*(it))->stop_ = true;
                (*(it))->cv_.notify_one();
                return;
            }
        }
        std::cerr << "Array list error: ereaseReadThread with non-exist id!" <<std::endl;
    }
};


#endif