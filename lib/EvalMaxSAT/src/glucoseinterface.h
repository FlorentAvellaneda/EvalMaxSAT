#ifndef MONGLUCOSE41_H
#define MONGLUCOSE41_H


#include "glucose/core/Solver.h"

#include "coutUtil.h"
#include "Chrono.h"

#include <thread>
#include <future>
#include <iostream>
#include <chrono>



class MonGlucose41 {
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

    ~MonGlucose41() {
        delete solver;
    }

    bool propagate(const std::vector<int> &assum, std::vector<int> &result) {
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

    void addClause(const std::vector<int> &vclause) {
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

    bool solve() {
        bool result = solver->solve();
        return result;
    }

    std::vector<int> getConflict() {
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


    std::vector<int> getConflict(const std::set<int> &assumptions) {
        std::vector<int> result;// = std::vector<int>(static_cast<unsigned int>(solver->conflict.size()));
        for(int i=0; i<solver->conflict.size(); i++) {
            int lit;
            if(Glucose::sign(solver->conflict[i])) {
                lit = -Glucose::var(solver->conflict[i]);
            } else {
                lit = Glucose::var(solver->conflict[i]);
            }

            if( assumptions.count(-lit) ) {
                result.push_back(-lit);
            }
        }
        return result;
    }


    unsigned int sizeConflict() {
        return solver->conflict.size();
    }


    template<class T>
    int solveLimited(const T &assumption, int confBudget, int except=0) {
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


    template<class T>
    int solveWithTimeoutAndLimit(const T &assumption, double timeout_sec, int confBudget, int except=0) {
        using namespace Glucose;

        // TODO: timeout

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

    template<class T>
    int solveWithTimeout(const T &assumption, double timeout_sec) {
        using namespace Glucose;

        // TODO: timeout

        Glucose::vec<Glucose::Lit> clause;

        for(auto e: assumption) {
            assert(e != 0);
            
            if(e < 0) {
                clause.push(Glucose::mkLit(-e, true));
            } else {
                clause.push(Glucose::mkLit(e, false));
            }
        }

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

    unsigned int nVars() {
        return solver->nVars() - 1;
    }

    bool solve(const std::vector<int> &assumption) {
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

    bool solve(const std::set<int> &assumption) {
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

    bool getValue(unsigned int var) {
        using namespace Glucose;
        return (solver->model[var] == l_True);
    }

    std::vector<bool> getSolution() {
        std::vector<bool> res = {false};

        for(unsigned int i=1; i<solver->model.size(); i++) {
            if(solver->model[i] == l_True) {
                res.push_back(true);
            } else {
                res.push_back(false);
            }
        }

        if(res.size() <= nVars()) {
            res.resize(nVars()+1, false);
        }

        return res;
    }

    unsigned int newVar(bool decisionVar=true) {
        using namespace Glucose;
        decisionVar=true;
        unsigned int var = static_cast<unsigned int>(solver->newVar(true, decisionVar));
        return var;
    }

    void setDecisionVar(unsigned int v, bool b) {
        solver->setDecisionVar(v, b);
    }

    unsigned int nClauses() {
        return solver->nClauses();
    }
};


#endif // MONGLUCOSE41_H
