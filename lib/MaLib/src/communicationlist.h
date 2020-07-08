#ifndef COMMUNICATIONLIST_LQSF093AEJL__H
#define COMMUNICATIONLIST_LQSF093AEJL___H

#include <list>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <cassert>
#include <iostream>


namespace MaLib {

template <class T>
class CommunicationList {
    std::mutex _mutex;

    std::condition_variable _cv_pop;
    std::condition_variable _cv_wait;
    bool newWaintingProcess = false;
    bool _closed = false;
    unsigned int numberWaiting=0;

    std::list<T> data;

public:

    CommunicationList() {

    }

    unsigned int getNumberWaiting() {
        std::lock_guard<std::mutex> lock(_mutex);
        return numberWaiting;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(_mutex);
        assert(numberWaiting == 0);
        _closed = false;
        newWaintingProcess = false;
        numberWaiting=0;
        data.clear();
    }

    void push(const T& element) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            assert(!_closed);
            data.push_back(element);
        }
        _cv_pop.notify_one();
    }

    template <class T2>
    void pushAll(const T2 &elements) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            assert(!_closed);
            data.insert(data.end(), elements.begin(), elements.end());
        }
        _cv_pop.notify_all();
    }

    /*
     * Blocks until a new element is added or the method close() is called.
     * return nothing if the CommunicationList is empty and closed.
     */
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(_mutex);
        if((data.size() == 0) && (_closed == false)) {
            ++numberWaiting;
            newWaintingProcess = true;
            _cv_wait.notify_all();

            _cv_pop.wait(lock, [&]{
                return (data.size() || _closed);
            });

            assert(numberWaiting > 0);
            --numberWaiting;
        }

        if(data.size()) {
            auto result = data.front();
            data.pop_front();
            return result;
        }
        assert(_closed);

        return {};
    }

    /*
     * No more elements will be added.
     * Processes waiting in pop() will be unlocked.
     */
    void close() {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _closed = true;
        }
        _cv_pop.notify_all();
    }

    unsigned int size() {
        std::lock_guard<std::mutex> lock(_mutex);
        return data.size();
    }

    /*
     * Wait until at least $n$ processes are wainting in pop().
     * If aware = true, then the function return the new number of processes wainting each time that new process becomes waiting in pop().
     */
    unsigned int wait(unsigned int n, bool aware = false) {

        std::unique_lock<std::mutex> lock(_mutex);
        assert(!_closed);

        if( std::max(0, static_cast<int>(numberWaiting) - static_cast<int>(data.size()) ) >= n ) {
            return std::max(0, static_cast<int>(numberWaiting) - static_cast<int>(data.size()) );
        }

        if(aware) {
            _cv_wait.wait(lock, [&]{
                return newWaintingProcess;
            });
        } else {
            _cv_wait.wait(lock, [&]{
                return std::max(0, static_cast<int>(numberWaiting) - static_cast<int>(data.size())) >= n;
            });
        }
        newWaintingProcess = false;
        return std::max(0, static_cast<int>(numberWaiting) - static_cast<int>(data.size()) );
    }

};

}


#endif // COMMUNICATIONLIST_LQSF093AEJL___H
