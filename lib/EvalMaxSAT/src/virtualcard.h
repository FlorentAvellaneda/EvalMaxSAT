#ifndef VIRTUALCARD_H
#define VIRTUALCARD_H

#include <sstream>
#include <vector>
#include <cassert>

class VirtualSAT;

class VirtualCard {


protected:
    VirtualSAT *solver;
    unsigned int nbLit;
    unsigned int bound;

    unsigned int newVar();
    void addClause(const std::vector<int> & clause);

    bool getValue(int lit);
    void setDecisionVar(unsigned int var, bool value);

public:

    VirtualCard(VirtualSAT *solver, const std::vector<int> &clause, unsigned int bound=0)
        : solver(solver), nbLit(static_cast<unsigned int>(clause.size())), bound(bound) {

    }

    virtual ~VirtualCard();

    virtual int operator <= (unsigned int k) {
        return atMost(k);
    }

    //virtual std::vector<int> getClause() {assert(!"TODO");}

    virtual unsigned int size() const {
        return nbLit;
    }

    virtual void print(std::ostream& os) const {
        os << "VirtualCard(size: "<<nbLit<<", bound: " << bound << ")";
    }

    virtual int atMost(unsigned int k) = 0;

};


#endif // VIRTUALCARD_H
