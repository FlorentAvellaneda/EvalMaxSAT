#pragma once

#include <random>
#include <cassert>
#include <iostream>
#include <algorithm>

namespace MaLib {
    class MonRand {
        static std::mt19937& getGen() {
            static std::mt19937 gen = std::mt19937();
            return gen;
        }
        
    public:
        template< class Sseq >
        static void seed(Sseq& seq) {
            MonRand::getGen().seed(seq);
        }

        static bool getBool(double proba=0.5) {
            assert(MonRand::getGen().min() == 0);
            auto v = MonRand::getGen().operator()();
            if( v > MonRand::getGen().max() / 2 ) {
                v--;
            }
            return v < MonRand::getGen().max() * proba;
        }

        static unsigned int get() {
            return MonRand::getGen().operator()();
        }

        static unsigned int get(unsigned int min, unsigned int max) {
            return min+MonRand::getGen().operator()()%(max - min + 1);
        }
        
        template <class T>
        static void shuffle(T &v) {
            std::shuffle(v.begin(), v.end(), getGen());
        }
        
        template <class RandomIt>
        static void shuffle(RandomIt first, RandomIt second) {
            std::shuffle(first, second, getGen());
        }
    };
}


