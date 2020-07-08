#ifndef VIRTUALMAXSAT_H
#define VIRTUALMAXSAT_H

#include <vector>
#include "virtualsat.h"

#include "glucose/utils/ParseUtils.h"

class VirtualMAXSAT : public VirtualSAT {
public:
    virtual ~VirtualMAXSAT();

    virtual unsigned int newSoftVar(bool value, bool decisionVar, unsigned int weight) = 0;

    virtual bool isSoft(unsigned int var) = 0;

    virtual void setVarSoft(unsigned int var, bool value, unsigned int weight=1) = 0;

    virtual int getCost() = 0;

    virtual void setNInputVars(unsigned int nb) = 0;

    int addWeightedClause(std::vector<int> clause, unsigned int weight) {
        assert(weight==1);
        if(clause.size() == 1) {
            if(!isSoft(abs(clause[0]))) {
                setVarSoft(abs(clause[0]), clause[0] > 0, weight);
                return clause[0];
            }
        }

        int r = static_cast<int>(newSoftVar(true, false, weight));
        clause.push_back(-r);
        addClause(clause);

        return r;
    }


    bool parse(gzFile in_) {
        Glucose::StreamBuffer in(in_);

        bool weighted = false;
        int64_t top = -1;
        int64_t weight = 1;

        std::vector<int> lits;
        int vars = 0;
        int inClauses = 0;
        int count = 0;
        for(;;) {
            Glucose::skipWhitespace(in);

            if(*in == EOF)
                break;

            if(*in == 'p') {
                ++in;
                if(*in != ' ') {
                    std::cerr << "o PARSE ERROR! Unexpected char: " << static_cast<char>(*in) << std::endl;
                    return false;
                }
                ++in;
                if(*in == 'w') { weighted = true; ++in; }

                if(Glucose::eagerMatch(in, "cnf")) {
                    vars = Glucose::parseInt(in);
                    setNInputVars(vars);
                    for(int i=0; i<vars; i++) {
                        newVar();
                    }
                    inClauses = Glucose::parseInt(in);
                    if(weighted && *in != '\n')
                        top = Glucose::parseInt64(in);
                } else {
                    std::cerr << "o PARSE ERROR! Unexpected char: " << static_cast<char>(*in) << std::endl;
                    return false;
                }
            }
            else if(*in == 'c')
                Glucose::skipLine(in);
            else {
                count++;
                if(weighted)
                    weight = Glucose::parseInt64(in);
                readClause(in, lits);
                if(weight >= top) {
                    addClause(lits);
                } else {
                    addWeightedClause(lits, weight);
                }
            }
        }
        if(count != inClauses) {
            std::cerr << "o WARNING! DIMACS header mismatch: wrong number of clauses." << std::endl;
            return false;
        }

        return true;
    }

private:

    template<class B>
    static void readClause(B& in, std::vector<int>& lits) {
        int parsed_lit;
        lits.clear();
        for (;;){
            parsed_lit = Glucose::parseInt(in);
            if (parsed_lit == 0) break;
            lits.push_back( parsed_lit );
        }
    }

};
VirtualMAXSAT::~VirtualMAXSAT() {}

#endif // VIRTUALMAXSAT_H
