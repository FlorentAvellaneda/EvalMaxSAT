#include "lazyvariable.h"

#include "coutUtil.h"
#include "virtualsat.h"


int LazyVariable::get() {
    if(!var) {
        assert(impliquants.size() > 0);

        if(impliquants.size() == 1) {
            assert(impliquants[0].size() > 0);
            if(impliquants[0].size() == 1 ) {
                var = impliquants[0][0]->get();
                return *var;
            }
        }

        var = solver->newVar(false);

        for(auto &implique: impliquants) {
            std::vector<int> clause;
            for(auto &lazyVar: implique) {
                int var = lazyVar->get();
                assert(var != 0);
                clause.push_back(-var);
            }
            assert(clause.size() > 0);
            clause.push_back(*var);
            solver->addClause(clause);
        }
    }

    return *var;
}


void LazyVariable::stopUse(VirtualSAT *solver) {

    assert(var);

    if(var) {
        assert(countUsed >= 1);
        countUsed--;
        if(countUsed == 0) {
            solver->setDecisionVar(abs(*var), false);
        }

        for(auto& implique: impliquants) {
            for(auto& lazyVar: implique) {
                assert(lazyVar);
                lazyVar->stopUse(solver);
            }
        }
    }
}


void LazyVariable::use(VirtualSAT *solver) {

    assert(var);

    if(var) {
        countUsed++;
        if(countUsed == 0) {
            solver->setDecisionVar(abs(*var), true);
        }

        for(auto& implique: impliquants) {
            for(auto& lazyVar: implique) {
                assert(lazyVar);
                lazyVar->use(solver);
            }
        }
    }
}

