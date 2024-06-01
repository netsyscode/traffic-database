#ifndef ARRAYLIST_HPP_
#define ARRAYLIST_HPP_
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <vector>
#include <condition_variable>
#include "util.hpp"

struct IDData{
    u_int8_t data;
    u_int8_t err;
};

template<class T>
struct ArrayListNode{
    T value;
    u_int32_t next; // if next > this->nodeNum, the next is in the other new array list
};

template<class T>
class ArrayList{
    const u_int32_t maxLength; // maxLength should be less than u_int32_t::max/2
    const u_int32_t warningLength; //once exceed warningLength, warning = true
    // std::atomic_bool warning;
    bool warning;
    std::atomic_uint32_t nodeNum; // now number of nodes, only increase

    std::atomic_uint32_t readThreadCount;
    std::atomic_uint32_t writeThreadCount;
    bool hasBegin;
    std::vector<ThreadPointer*> readThreads;
    std::shared_mutex readThreadsMutex;

    u_int8_t* idArray;
    ArrayListNode<T>* array;
public:
    ArrayList(u_int32_t maxLength, u_int32_t warningLength):maxLength(maxLength),warningLength(warningLength){
        this->array = new ArrayListNode<T>[maxLength];
        this->idArray = new u_int8_t[maxLength]();
        this->nodeNum = 0;
        this->warning = false;
        this->readThreadCount = 0;
        this->writeThreadCount = 0;
        this->hasBegin = false;
    }
    ~ArrayList(){
        if(this->readThreadCount || this->writeThreadCount){
            std::cout << "Array list warning: it is used by " << this->readThreadCount + this->writeThreadCount << " thread." <<std::endl;
        }
        delete[] array;
        delete[] idArray;
    }
    u_int32_t addNodeMultiThread(T value, u_int8_t id){
        u_int32_t num = num++;
        if(num >= this->maxLength){
            printf("Array list error: addNodeOneThread overflow the buffer: %u-%u!\n",num,this->maxLength);
            return std::numeric_limits<uint32_t>::max();
        }
        this->array[num].value = value;
        this->array[num].next = std::numeric_limits<uint32_t>::max();
        this->idArray[num] = id == 0? 1 : id;
        if(num >= this->warningLength){
            this->warning = true;
        }
        return num;
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
        this->writeThreadCount++;
    }
    void ereaseWriteThread(){
        this->writeThreadCount--;
        this->hasBegin = true;
    }
    bool addReadThread(ThreadPointer* thread){
        std::unique_lock<std::shared_mutex> lock(this->readThreadsMutex);
        for(auto t:this->readThreads){
            if(t->id == thread->id){
                std::cerr << "Array list error: addReadThread with exist id!" <<std::endl;
                lock.unlock();
                return false;
            }
        }
        thread->stop_ = false;
        thread->pause_ = false;
        this->readThreads.push_back(thread);
        this->readThreadCount++;
        lock.unlock();
        return true;
    }
    bool ereaseReadThread(ThreadPointer* thread){
        std::unique_lock<std::shared_mutex> lock(this->readThreadsMutex);
        for(auto it = this->readThreads.begin();it!=this->readThreads.end();++it){
            if((*(it))->id == thread->id){
                this->readThreads.erase(it);
                this->readThreadCount--;
                lock.unlock();
                return true;
            }
        }
        std::cerr << "Array list error: ereaseReadThread with non-exist id: " << thread->id << "!" <<std::endl;
        lock.unlock();
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
        if(this->idArray[pos]!=0){
            data.data = this->idArray[pos];
            return data;
        }

        ThreadPointer* thread = nullptr;
        std::shared_lock<std::shared_mutex> rlock(this->readThreadsMutex);
        for(auto t:this->readThreads){
            if(t->id == thread_id){
                thread = t;
                break;
            }
        }
        rlock.unlock();
        if(thread == nullptr){
            std::cerr << "Array list error: getID with non-exist thread id!" <<std::endl;
            data.err = 2;
            return data;
        }

        while (true){
            if(this->idArray[pos]!=0 || thread->stop_ || (pos > this->warningLength && thread->pause_) || (this->hasBegin && this->writeThreadCount == 0 && thread->pause_)){
                break;
            }
        }
        
        if(thread->stop_){
            std::cout << "Array list log: thread " << thread_id << " stop." <<std::endl;
            data.err = 3;
            return data;
        }
        if(this->idArray[pos]!=0){
            data.data = this->idArray[pos];
            return data;
        }
        //pause and read all data
        this->nodeNum = pos;
        // printf("Array list log: pos is %u, node num %u, id %u.\n",pos,this->nodeNum.load(),this->idArray[pos]);
        data.err = 4;
        return data;
    }
    // Non-parallelizable, get ID from certain pos
    IDData getIDOneThread(u_int32_t pos){
        IDData data = {
            .data = 0,
            .err = 0,
        };
        if(pos > this->maxLength || pos < 0){
            std::cerr << "Array list error: getIDOneThread overflow the buffer!" <<std::endl;
            data.err = 1;
            return data;
        }
        if(pos > this->nodeNum){
            std::cerr << "Array list error: getIDOneThread overflow the node number!" <<std::endl;
            data.err = 2;
            return data;
        }
        if(this->ReadThreadCount){
            std::cerr << "Array list error: getIDOneThread while it is used by certain thread!" <<std::endl;
            data.err = 3;
            return data;
        }
        
        data.data = this->idArray[pos];
        return data;
    }
    // Parallelizable and read only, get value from certain pos, each read thread read on certain id
    T getValue(u_int32_t pos, u_int8_t id){
        // if(pos >= this->warningLength){
        //     printf("Array list log: pos is %u.\n",pos);
        // }
        if(pos > this->maxLength || pos < 0){
            std::cerr << "Array list error: getValue overflow the buffer!" <<std::endl;
            return T();
        }
        // if(pos >= this->nodeNum){
        if(this->idArray[pos] == 0){
            std::cerr << "Array list error: getValue overflow the node count!" <<std::endl;
            return T();
        }
        if(id != this->idArray[pos]){
            std::cerr << "Array list error: getValue on wrong ID: "<<(int)id << " - " << (int)(this->idArray[pos]) << "!"<<std::endl;
            std::cerr << pos <<std::endl;
            return T();
        }
        // std::unique_lock<std::shared_mutex> lock(mutex_);
        return this->array[pos].value;
    }
    // read only, for querier
    T getValueReadOnly(u_int32_t pos)const{
        if(pos > this->maxLength || pos < 0){
            std::cerr << "Array list error: getValue overflow the buffer!" <<std::endl;
            return T();
        }
        if(pos >= this->nodeNum){
            std::cerr << "Array list error: getValue overflow the node count!" <<std::endl;
            return T();
        }
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
    //output with copy
    std::string outputToChar()const{
        std::string data = std::string();
        if(this->readThreadCount || this->writeThreadCount){
            std::cerr << "Array list error: outputToChar while it is used by certain thread!" <<std::endl;
            return data;
        }
        u_int32_t len = this->getLength();
        // if(this->warning){
        //     len = (this->warningLength + 1)*sizeof(ArrayListNode<T>);
        // }
        data = std::string((char*)this->array, len);
        return data;
    }
    void asynchronousStop(u_int32_t threadID){
        for(auto it = this->readThreads.begin();it!=this->readThreads.end();++it){
            if((*(it))->id == threadID){
                (*(it))->stop_ = true;
                (*(it))->cv_.notify_one();
                return;
            }
        }
        std::cerr << "Array list error: asynchronousStop with non-exist id " << threadID << "!" <<std::endl;
    }
    void asynchronousPause(u_int32_t threadID){
        for(auto it = this->readThreads.begin();it!=this->readThreads.end();++it){
            if((*(it))->id == threadID){
                (*(it))->pause_ = true;
                (*(it))->cv_.notify_one();
                return;
            }
        }
        std::cerr << "Array list error: asynchronousPause with non-exist id!" <<std::endl;
    }
    // void getWarningValue(){
    //     if(!this->warning){
    //         printf("Array list error: getWarningValue without warning!\n");
    //     }
    //     return this->array[this->warningLength].value;
    // }
};


#endif