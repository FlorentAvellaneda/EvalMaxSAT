#ifndef MONGLUCOSE41_H
#define MONGLUCOSE41_H

#include "virtualsat.h"

#include "glucose/core/Solver.h"

#include "coutUtil.h"
#include "Chrono.h"

#include <thread>
#include <future>
#include <iostream>
#include <chrono>



class MonGlucose41 : public VirtualSAT {
    Glucose::Solver *solver;

    MonGlucose41(Glucose::Solver* solver)
        : solver(solver) {
    }
public:

    MonGlucose41()
        : solver(new Glucose::Solver()) {
        solver->verbosity = 0;
        solver->newVar(false);
        solver->setDecisionVar(0, false);

        //solver->setIncrementalMode();
    }

    VirtualSAT* clone() override {
        auto result = new MonGlucose41(static_cast<Glucose::Solver*>(solver->clone()));
        return result;
    }

    ~MonGlucose41() override;

    bool propagate(const std::vector<int> &assum, std::vector<int> &result) override {
        Glucose::vec<Glucose::Lit> clause;

        for(auto e: assum) {
            assert(e != 0);
            if(e < 0) {
                clause.push(Glucose::mkLit(-e, true));
            } else {
                clause.push(Glucose::mkLit(e, false));
            }
        }

        Glucose::vec<Glucose::Lit> resultGlu;

        if(solver->prop_check(clause, resultGlu, 2)) {
            result.resize(resultGlu.size());
            for(unsigned int i=0; i<result.size(); i++) {
                if( Glucose::sign( resultGlu[i] ) ) { // Negatif
                    result[i] = -Glucose::var(resultGlu[i]);
                } else {
                    result[i] = Glucose::var(resultGlu[i]);
                }
            }
            return true;
        }

        return false;
    }

    void addClause(const std::vector<int> &vclause) override {
        Glucose::vec<Glucose::Lit> clause;

        for(auto e: vclause) {
            if(e < 0) {
                clause.push(Glucose::mkLit(-e, true));
            } else if (e  > 0) {
                clause.push(Glucose::mkLit(e, false));
            } else {
                assert(!"litteral = 0!!");
            }
        }

        solver->addClause(clause);
    }

    bool solve() override {
        bool result = solver->solve();
        return result;
    }

    std::vector<int> getConflict() override {
        std::vector<int> result = std::vector<int>(static_cast<unsigned int>(solver->conflict.size()));
        for(int i=0; i<solver->conflict.size(); i++) {
            if(Glucose::sign(solver->conflict[i])) {
                result[static_cast<unsigned int>(i)] = -Glucose::var(solver->conflict[i]);
            } else {
                result[static_cast<unsigned int>(i)] = Glucose::var(solver->conflict[i]);
            }
        }
        return result;
    }

    unsigned int sizeConflict() override {
        return solver->conflict.size();
    }

    int solveLimited(const std::vector<int> &assumption, int confBudget, int except=0) override {
        using namespace Glucose;

        Glucose::vec<Glucose::Lit> clause;

        for(auto e: assumption) {
            assert(e != 0);
            if(e==except)
                continue;
            if(e < 0) {
                clause.push(Glucose::mkLit(-e, true));
            } else {
                clause.push(Glucose::mkLit(e, false));
            }
        }

        solver->setConfBudget(confBudget);

        auto result = solver->solveLimited(clause);

        if(result==l_True) {
            return 1;
        }
        if(result==l_False) {
            return -1;
        }
        if(result==l_Undef) {
            return 0;
        }

        assert(false);
    }

    int solveLimited(const std::list<int> &assumption, int confBudget, int except=0) override {
        using namespace Glucose;

        Glucose::vec<Glucose::Lit> clause;

        for(auto e: assumption) {
            assert(e != 0);
            if(e==except)
                continue;
            if(e < 0) {
                clause.push(Glucose::mkLit(-e, true));
            } else {
                clause.push(Glucose::mkLit(e, false));
            }
        }

        solver->setConfBudget(confBudget);

        auto result = solver->solveLimited(clause);

        if(result==l_True) {
            return 1;
        }
        if(result==l_False) {
            return -1;
        }
        if(result==l_Undef) {
            return 0;
        }

        assert(false);
        return 0;
    }


    int solveLimited(const std::set<int> &assumption, int confBudget, int except) override {
        using namespace Glucose;
        Glucose::vec<Glucose::Lit> clause;

        for(auto e: assumption) {
            assert(e != 0);
            if(e==except)
                continue;
            if(e < 0) {
                clause.push(Glucose::mkLit(-e, true));
            } else {
                clause.push(Glucose::mkLit(e, false));
            }
        }

        solver->setConfBudget(confBudget);

        auto result = solver->solveLimited(clause);

        if(result==l_True) {
            return 1;
        }
        if(result==l_False) {
            return -1;
        }
        if(result==l_Undef) {
            return 0;
        }

        assert(false);
        return 0;
    }

    unsigned int nVars() override {
        return solver->nVars() - 1;
    }

    bool solve(const std::vector<int> &assumption) override {
        Glucose::vec<Glucose::Lit> clause;
        for(auto e: assumption) {
            assert(e != 0);
            if(e < 0) {
                clause.push(Glucose::mkLit(-e, true));
            } else {
                clause.push(Glucose::mkLit(e, false));
            }
        }

        bool result = solver->solve(clause);

        return result;
    }

    bool getValue(unsigned int var) override {
        using namespace Glucose;
        return (solver->model[var] == l_True);
    }

    unsigned int newVar(bool decisionVar=true) override {
        using namespace Glucose;
        decisionVar=true;
        unsigned int var = static_cast<unsigned int>(solver->newVar(true, decisionVar));
        return var;
    }

    void setDecisionVar(unsigned int v, bool b) override {
        solver->setDecisionVar(v, b);
    }

    unsigned int nClauses() override {
        return solver->nClauses();
    }
};
MonGlucose41::~MonGlucose41() {
    delete solver;
}


#endif // MONGLUCOSE41_H
