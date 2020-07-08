#ifndef VIRTUALSAT_H
#define VIRTUALSAT_H

#include <vector>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <memory>
#include <iostream>

#include "cardincremental.h"
#include "card_oe.h"
#include "lazyvariable.h"
#include "coutUtil.h"

using namespace MaLib;

class VirtualSAT {
public:

    virtual ~VirtualSAT();

    virtual VirtualSAT* clone() {assert(!"TODO");}

    virtual void addClause(const std::vector<int> &vclause)  {assert(!"TODO");}

    virtual unsigned int nVars() {assert(!"TODO");}

    virtual unsigned int nSoftVar() {assert(!"TODO");}

    virtual unsigned int nClauses() {assert(!"TODO");}

    virtual bool solve() {assert(!"TODO");}

    virtual void setDecisionVar(unsigned int v, bool b) {assert(!"TODO");}

    virtual bool propagate(const std::vector<int> &assum, std::vector<int> &result) {assert(!"TODO");}

    virtual bool solve(const std::vector<int> &assumption)  {assert(!"TODO");}

    virtual int solveLimited(const std::vector<int> &assumption, int confBudget, int except=0)  {assert(!"TODO");}

    virtual int solveLimited(const std::list<int> &assumption, int confBudget, int except=0)  {assert(!"TODO");}

    virtual int solveLimited(const std::set<int> &assumption, int confBudget, int except=0)  {assert(!"TODO");}

    virtual bool getValue(unsigned int var)  {assert(!"TODO");} // TODO: unsigned int

    virtual unsigned int newVar(bool decisionVar=true) {assert(!"TODO");}

    virtual unsigned int sizeConflict() {assert(!"TODO");}

    virtual std::vector<int> getConflict()  {assert(!"TODO");}

    ////////////////
    /// For CARD ///
    ////////////////
    virtual void AtMostOne(const std::vector<int> &vclause) {
        if(vclause.size() <= 2) // Hyperparametre
            AtMostOne_Pairwise(vclause);
        AtMostOne_Bimander(vclause);
    }

    void AtMostOne_Bimander(const std::vector<int> &vclause) {
        //double tmp = sqrt(vclause.size());
        double tmp;// = 3.0;//vclause.size()/4.0;

        if(vclause.size() >= 10)    // Hyperparametre
            tmp = vclause.size()/2.0;
        else
            tmp = sqrt(vclause.size());

        unsigned int g = static_cast<unsigned int>(tmp);
        g += ((tmp-g) > 0);

        std::vector<std::vector<int>> subclause;
        unsigned int i=0;
        while(i < vclause.size()) {
            if((i % g) == 0) {
                subclause.push_back(std::vector<int>());
            }
            subclause.back().push_back(vclause[i]);
            i++;
        }
        for(unsigned int i=0; i<subclause.size(); i++) {
            AtMostOne_Binary( subclause[i] );
            //AtMostOne_Pairwise( subclause[i] );
        }

        double nbBitd = log2(subclause.size());
        unsigned int nbBit = static_cast<unsigned int>(nbBitd);//log2(subclause.size())+0.5);
        nbBit += ((nbBitd - nbBit) > 0);

        int alt=1;
        for(unsigned int j=0; j<nbBit; j++) {
            unsigned int B = newVar();
            bool isFault = true;

            for(unsigned int i=0; i<subclause.size(); i++) {

                if(static_cast<int>(i)%alt == 0)
                    isFault = !isFault;

                for(unsigned int h=0; h<subclause[i].size(); h++) {
                    if(isFault) {
                        addClause({-subclause[i][h], static_cast<int>(B)});
                    } else {
                        addClause({-subclause[i][h], -static_cast<int>(B)});
                    }
                }
            }

            alt *= 2;
        }
    }

    // Pairwise Encoding
    void AtMostOne_Pairwise(const std::vector<int> &vclause) {
        for(unsigned int i=0; i<vclause.size(); i++) {
            for(unsigned int j=i+1; j<vclause.size(); j++) {
                addClause({-vclause[i], -vclause[j]});
            }
        }
    }

    // Binary Encoding
    void AtMostOne_Binary(const std::vector<int> &vclause) {
        double nbBitd = log2(vclause.size());
        unsigned int nbBit = static_cast<unsigned int>(nbBitd);//log2(subclause.size())+0.5);
        nbBit += ((nbBitd - nbBit) > 0);

        int alt=1;
        for(unsigned int j=0; j<nbBit; j++) {
            unsigned int B = newVar();
            bool isFault = true;

            for(unsigned int i=0; i<vclause.size(); i++) {
                if(static_cast<int>(i)%alt == 0)
                    isFault = !isFault;
                if(isFault) {
                    addClause({-vclause[i], static_cast<int>(B)});
                } else {
                    addClause({-vclause[i], -static_cast<int>(B)});
                }
            }

            alt *= 2;
        }
    }

    std::shared_ptr<VirtualCard> newCard(const std::vector<int> &clause, unsigned int bound=1) {
        return std::make_shared<CardIncremental_Lazy>(this, clause, bound);
        //return std::make_shared<Card_Lazy_OE>(this, clause); // Fonctionne moins bien
    }

    std::shared_ptr<LazyVariable> newLazyVariable() {
        return LazyVariable::newVar(this);
    }


};



#endif // VIRTUALSAT_H
