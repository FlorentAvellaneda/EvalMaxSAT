#pragma once

#define CHRONO_R7U52KTM

#include <sys/time.h>
#include <cstdio>
#include <iostream>
#include <chrono>
#include <cassert>
#include <memory>

namespace MaLib
{
    class Chrono {
        class AddTime {
            Chrono* C;
        public:
            AddTime(Chrono *C) : C(C) {
                assert(C->_pause == true);
                C->pause(false);
            }

            ~AddTime() {
                assert(C->_pause == false);
                C->pause(true);
            }
        };

        std::string _name;
        bool _showWhenDestroyed=false;
        long long duree_nano = 0;
        std::chrono::time_point<std::chrono::system_clock> start;
        bool _pause = false;
    public:
        Chrono(std::string name, bool showWhenDestroyed=true) : _name(name), _showWhenDestroyed(showWhenDestroyed) {
            start = std::chrono::system_clock::now();
        }

        Chrono() {
            start = std::chrono::system_clock::now();
        }

        ~Chrono() {
            if(_showWhenDestroyed) {
                print();
            }
        }

        void setShowWhenDestroyed(bool showWhenDestroyed) {
            _showWhenDestroyed = showWhenDestroyed;
        }

        std::unique_ptr<AddTime> addTime() {
            return std::make_unique<AddTime>(this);
        }

        void print() {
            if(_name != "") {
                std::cout << _name << " : ";
            }

            std::cout << *this << std::endl;
        }

        void pause(bool v=true) {
            if(_pause == v) return;
            _pause = v;

            if(v) {
                std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
                std::chrono::duration<long long, std::nano> elapsed = end - start;
                duree_nano += elapsed.count();
            } else {
                start = std::chrono::system_clock::now();
            }
        }

        void tic() {
            start = std::chrono::system_clock::now();
            duree_nano = 0;
            _pause = false;
        }

        double tacSec() const {
            std::chrono::duration<double> elapsed_seconds = std::chrono::nanoseconds(duree_nano);

            if(!_pause) {
            std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
                elapsed_seconds += (end - start) ;
            }

            return elapsed_seconds.count();
        }

        friend std::ostream& operator<<(std::ostream& os, const Chrono& C);
    };

    inline std::ostream& operator<<(std::ostream& os, const Chrono& C)  {
        os << C.tacSec() << "s";
        return os;
    }

    class TimeOut {
        Chrono C;
        double val;
    public:
        TimeOut(double seconds) : val(seconds) {

        }

        void restart() {
            C.tic();
        }

        void pause(bool v) {
            C.pause(v);
        }

        bool operator() () const {
            return C.tacSec() >= val;
        }

        double timeLeft() const {
            return val - C.tacSec();
        }

        double getCoefLeft() const {
            return 1 - (C.tacSec() / val);
        }

        double getCoefPast() const {
            return C.tacSec() / val;
        }

        double getVal() const {
            return val;
        }


    };

}


