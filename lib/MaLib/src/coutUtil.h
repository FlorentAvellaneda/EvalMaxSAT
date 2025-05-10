#pragma once


#include <iostream>
#include <vector>
#include <list>
#include <deque>
#include <memory>
#include <array>
#include <set>
#include <optional>
#include <tuple>
#include <map>
#include <sstream>
#include <string>
#include <mutex>

#include "Chrono.h"

namespace MaLib
{

// Afficher le continue d'un vector
template <class T>
std::ostream& operator<< (std::ostream &output, const std::vector<T> &v) {
    unsigned int size=v.size();
    output << "[";
    for(unsigned int i=0; i<size; i++) {
        output << v[i];
        if( i+1 < size)
            output << ", ";
    }
    output << "]";
    return output;
}


// Afficher le continue d'un array
template <class T, unsigned long I>
std::ostream& operator<< (std::ostream &output, const std::array<T, I> &v) {
    unsigned int size=v.size();
    output << "[";
    for(unsigned int i=0; i<size; i++) {
        output << v[i];
        if( i+1 < size)
            output << ", ";
    }
    output << "]";
    return output;
}

// Afficher le continue d'une list
template <class T>
std::ostream& operator<< (std::ostream &output, const std::list<T> &v) {
    bool first=true;
    output << "[";
    //for(unsigned int i=0; i<size; i++) {
    for(auto &e: v) {
        if(!first) {
            output << ", ";
        } else {
            first=false;
        }
        output << e;
    }
    output << "]";
    return output;
}

// Afficher le continue d'un deque
template <class T>
std::ostream& operator<< (std::ostream &output, const std::deque<T> &v) {
    unsigned int size=v.size();
    output << "[";
    for(unsigned int i=0; i<size; i++) {
        output << v[i];
        if( i+1 < size)
            output << ", ";
    }
    output << "]";
    return output;
}

// Afficher le continue d'un set
template <class T>
std::ostream& operator<< (std::ostream &output, const std::set<T> &s) {
	output << "{";
	bool first=true;
	for(auto &e: s) {
		if(!first)
			output << ", ";
		else
    	first=false;
		output << e;
	}
	output << "}";
	return output;
}

namespace {
	template<std::size_t> struct int_{};

	template <class Tuple, size_t Pos>
	std::ostream& print_tuple(std::ostream& out, const Tuple& t, int_<Pos> ) {
	  out << std::get< std::tuple_size<Tuple>::value-Pos >(t) << ", ";
	  return print_tuple(out, t, int_<Pos-1>());
	}

	template <class Tuple>
	std::ostream& print_tuple(std::ostream& out, const Tuple& t, int_<1> ) {
	  return out << std::get<std::tuple_size<Tuple>::value-1>(t);
	}
}

// Afficher le continue d'un tuple
template <class... Args>
std::ostream& operator<< (std::ostream &output, const std::tuple<Args...> &t) {
    output << "<";
    print_tuple(output, t, int_<sizeof...(Args)>()); 
    output << ">";
    return output;
}

template <class T1, class T2>
std::ostream& operator<< (std::ostream &output, const std::pair<T1, T2> &p) {
    output << "(" << p.first << ", " << p.second << ")";
    return output;
}


// Afficher le continue d'un map
template <class T1, class T2>
std::ostream& operator<< (std::ostream &output, const std::map<T1, T2> &s) {
	output << "{";
	bool first=true;
	for(auto &e: s) {
		if(!first)
			output << ", ";
		else
    	first=false;
		output << e.first << ": " << e.second;
	}
	output << "}";
	return output;
}

namespace  {
    void _toString(std::ostringstream &oss){
    }

    template<typename T1, typename ...T>
    void _toString(std::ostringstream &oss, const T1& arg, const T&... args) {
        oss << arg;
        _toString(oss, args...);
    }
}

template<typename ...T>
std::string toString(const T&... args) {
    std::ostringstream oss;
    _toString(oss, args...);
    return oss.str();
}

namespace {
#ifdef NDEBUG
    //MaLib::Chrono MonPrint_Chrono;
#else
    MaLib::Chrono MonPrint_Chrono;
#endif
}
template<typename ...T>
void MonPrint(const T&... args) {
#ifdef NDEBUG
    //std::cout << "c " << MonPrint_Chrono.tacSec() << ": " << toString(args...) << std::endl;
#else
    std::cout << "c " << (int)(MonPrint_Chrono.tacSec()) << ": " << toString(args...) << std::endl;
#endif
}


}


