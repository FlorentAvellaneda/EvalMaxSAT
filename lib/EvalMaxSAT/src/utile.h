#pragma once

#include <vector>
#include <cmath>
#include <cassert>
#include <tuple>
#include <zlib.h>

#include "ParseUtils.h"


typedef unsigned long long int t_weight;

class WeightVector {
    std::vector<long long int> _weight;
public:
    long long int operator[](int lit) {
        assert(abs(lit) < _weight.size());
        return (_weight[abs(lit)] ^ (lit >> 31)) - (lit >> 31);
    }

    unsigned int size() const {
        return _weight.size();
    }

    void resize(unsigned int newSize) {
        _weight.resize(newSize);
    }

    void push_back(long long int v) {
        _weight.push_back(v);
    }

    void set(int lit, long long int weight) {
        assert(abs(lit) < _weight.size());
        if(lit < 0 ) {
            _weight[-lit] = -weight;
        } else {
            _weight[lit] = weight;
        }
    }

    void add(int lit, long long int weight) {
        assert(abs(lit) < _weight.size());
        if(lit < 0 ) {
            _weight[-lit] += -weight;
        } else {
            _weight[lit] += weight;
        }
    }
};

template<class T>
class doublevector {

    std::vector<T> posIndexVector;
    std::vector<T> negIndexVector;

public:

    void push_back(const T& v) {
        posIndexVector.push_back(v);
    }

    T& back() {
        if(posIndexVector.size()) {
            return posIndexVector.back();
        } else {
            return negIndexVector[0];
        }
    }

    void pop_back() {
        if(posIndexVector.size()) {
            posIndexVector.pop_back();
        } else {
            negIndexVector.erase(negIndexVector.begin());
        }
    }

    void add(int index, const T& val) {
        if(index >= 0) {
            if(index >= posIndexVector.size()) {
                posIndexVector.resize(index+1);
            }
            posIndexVector[index] = val;
        } else {
            if(-index >= negIndexVector.size()) {
                negIndexVector.resize((-index)+1);
            }
            negIndexVector[-index] = val;
        }
    }

    T& operator [](int index) {
        if(index >= 0) {
            assert( index < posIndexVector.size() );
            return posIndexVector[index];
        } else {
            assert( -index < negIndexVector.size() );
            return negIndexVector[-index];
        }
        assert(false);
    }

    T& get(int index) {
        if(index >= 0) {
            if(index >= posIndexVector.size()) {
                posIndexVector.resize(index+1);
            }
            return posIndexVector[index];
        } else {
            if(-index >= negIndexVector.size()) {
                negIndexVector.resize((-index)+1);
            }
            return negIndexVector[-index];
        }
    }


};


/// POUR DEBUG ////
template<class B>
static void readClause(B& in, std::vector<int>& lits) {
    int parsed_lit;
    lits.clear();
    for (;;){
        parsed_lit = parseInt(in);
        if (parsed_lit == 0) break;
        lits.push_back( parsed_lit );
    }
}

/// POUR DEBUG ////
static t_weight calculateCost(const std::string & file, std::vector<bool> &result) {
    t_weight cost = 0;
    auto in_ = gzopen(file.c_str(), "rb");
    t_weight weightForHardClause = -1;

    StreamBuffer in(in_);

    std::vector<int> lits;
    for(;;) {
        skipWhitespace(in);

        if(*in == EOF)
            break;

        else if(*in == 'c') {
            skipLine(in);
        } else if(*in == 'p') { // Old format
          ++in;
          if(*in != ' ') {
              std::cerr << "c PARSE ERROR! Unexpected char: " << static_cast<char>(*in) << std::endl;
              return false;
          }
          skipWhitespace(in);

          if(eagerMatch(in, "wcnf")) {
              parseInt(in); // # Var
              parseInt(in); // # Clauses
              weightForHardClause = parseWeight(in);
          } else {
              std::cerr << "c PARSE ERROR! Unexpected char: " << static_cast<char>(*in) << std::endl;
              return false;
          }
      }
        else {
            t_weight weight = parseWeight(in);
            readClause(in, lits);
            if(weight >= weightForHardClause) {
                bool sat=false;
                for(auto l: lits) {
                    if(abs(l) >= result.size()) {
                        result.resize(abs(l)+1, 0);
                        assert(!"Error size result");
                    }
                    if ( (l>0) == (result[abs(l)]) ) {
                        sat = true;
                        break;
                    }
                }
                if(!sat) {
                    //std::cerr << "Error : solution no SAT !" << std::endl;
                    return -1;
                }
            } else {
                bool sat=false;
                for(auto l: lits) {
                    if(abs(l) >= result.size()) {
                        result.resize(abs(l)+1, 0);
                        assert(!"Error size result");
                    }

                    if ( (l>0) == (result[abs(l)]) ) {
                        sat = true;
                        break;
                    }
                }
                if(!sat) {
                    cost += weight;
                }
            }
        }
    }

    gzclose(in_);
    return cost;
}


template<class MAXSAT_SOLVER>
static std::vector<int> readClause(StreamBuffer &in, MAXSAT_SOLVER* solveur) {
    std::vector<int> clause;

    for (;;) {
        int lit = parseInt(in);

        if (lit == 0)
            break;
        clause.push_back(lit);
        while( abs(lit) > solveur->nVars()) {
            solveur->newVar();
        }
    }

    return clause;
}

template<class MAXSAT_SOLVER>
static bool parse(const std::string& filePath, MAXSAT_SOLVER* solveur) {
    auto gz = gzopen( filePath.c_str(), "rb");

    StreamBuffer in(gz);
    t_weight weightForHardClause = -1;

    if(*in == EOF) {
        return false;
    }

    std::vector < std::tuple < std::vector<int>, t_weight> > softClauses;

    for(;;) {
        skipWhitespace(in);

        if(*in == EOF) {
            break;
        }

        if(*in == 'c') {
            skipLine(in);
        } else if(*in == 'p') { // Old format
            ++in;
            if(*in != ' ') {
                std::cerr << "c PARSE ERROR! Unexpected char: " << static_cast<char>(*in) << std::endl;
                return false;
            }
            skipWhitespace(in);

            if(eagerMatch(in, "wcnf")) {
                parseInt(in); // # Var
                parseInt(in); // # Clauses
                weightForHardClause = parseWeight(in);
            } else {
                std::cerr << "c PARSE ERROR! Unexpected char: " << static_cast<char>(*in) << std::endl;
                return false;
            }
        } else {
            t_weight weight = parseWeight(in);
            std::vector<int> clause = readClause(in, solveur);

            if(weight >= weightForHardClause) {
                solveur->addClause(clause);
            } else {
                // If it is a soft clause, we have to save it to add it once we are sure we know the total number of variables.
                softClauses.push_back({clause, weight});
            }
        }
    }

    solveur->setNInputVars(solveur->nVars());
    for(auto & [clause, weight]: softClauses) {
        solveur->addClause(clause, weight);
    }

    gzclose(gz);
    return true;
 }


static std::vector<int> readClause(StreamBuffer &in) {
    std::vector<int> clause;

    for (;;) {
        int lit = parseInt(in);
        if (lit == 0)
            break;
        clause.push_back(lit);
    }

    return clause;
}



