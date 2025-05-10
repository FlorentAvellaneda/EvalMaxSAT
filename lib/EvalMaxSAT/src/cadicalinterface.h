#pragma once


#include "cadical.hpp"
#include "internal.hpp"

#include <cmath>
#include <cassert>
#include <vector>
#include <set>


class Solver_cadical {
    CaDiCaL::Solver *solver;
    unsigned int nVar=0;
public:

    Solver_cadical() : solver(new CaDiCaL::Solver()) {}

    ~Solver_cadical() {
        delete solver;
    }

    template<class T>
    void exportClauses(T& to) const {
        for (auto idx : solver->internal->vars) {
            const int tmp = solver->internal->fixed (idx);
            if (tmp) to.addClause({tmp < 0 ? -idx : idx});
        }
        for (const auto & c : solver->internal->clauses) {
            if (!c->garbage) {
                std::vector<int> cl;
                for (const auto & lit : *c) {
                    cl.push_back(lit);
                }
                to.addClause(cl);
            }
        }
    }

    bool getValue(int lit) {
        return solver->val(lit) > 0;
    }

    std::vector<bool> getSolution() {
        std::vector<bool> res = solver->getSolution();
        if(res.size() <= nVar) {
            res.resize(nVar+1, false);
        }
        return res;
    }

    unsigned int nVars() {
        return nVar;
    }

    int newVar(bool decisionVar=true) {
         // decisionVar not implemented in Cadical ?
         return ++nVar;
     }

    void addClause(const std::vector<int> &clause) {
        for (int lit : clause) {
            if( abs(lit) > nVar) {
                nVar = abs(lit);
            }
            solver->add(lit);
        }
       solver->add(0);
    }

    void simplify() {
        solver->simplify();
    }

    bool solve(const std::vector<bool>& solution) {
        for(unsigned int i=1; i<solution.size(); i++) {
            if(solution[i]) {
                solver->assume(i);
            } else {
                solver->assume(-(int)i);
            }
        }

        int result = solver->solve();

        if( !( (result == 10) || (result == 20) ) ) {
            return solve(solution);
        }

        return result == 10; // Sat
    }

    bool solve() {
        int result = solver->solve();

        //assert( (result == 10) || (result == 20) ); // Bug? Can happen sometimes...
        if( !( (result == 10) || (result == 20) ) ) {
            return solve();
        }

        return result == 10; // Sat
    }

    template<class T>
    bool solve(const T &assumption) {

        for (int lit : assumption) {
            solver->assume(lit);
        }

        int result = solver->solve();

        //assert( (result == 10) || (result == 20) ); // Bug? Can happen sometimes...
        if( !( (result == 10) || (result == 20) ) ) {
            return solve(assumption);
        }
        return result == 10; // Sat
    }

    template<class T>
    bool solve(const T &assumption, const std::set<int> &forced) {
        for (int lit : forced) {
            solver->assume(lit);
        }
        for (int lit : assumption) {
            if(forced.count(-lit) == 0) {
                solver->assume(lit);
            }
        }

        int result = solver->solve();

        //assert( (result == 10) || (result == 20) ); // Bug? Can happen sometimes...
        if( !( (result == 10) || (result == 20) ) ) {
            return solve(assumption, forced);
        }

        return result == 10; // Sat
    }

    template<class T>
    int solveLimited(const T &assumption, int confBudget, int except=0) {
        for (int lit : assumption) {
            if (lit == except)
                continue;
            solver->assume(lit);
        }

        solver->limit("conflicts", confBudget);

        auto result = solver->solve();

        if(result==10) { // Satisfiable
            return 1;
        }
        if(result==20) { // Unsatisfiable
            return 0;
        }
        if(result==0) { // Limit
            return -1;
        }

        assert(false);
        return 0;
    }


    template<class T>
    int solveWithTimeout(const T &assumption, double timeout_sec, int except=0) {
        solver->reset_assumptions();

        for (int lit : assumption) {
            if (lit == except)
                continue;
            solver->assume(lit);
        }

        solver->setTimeout(timeout_sec);

        auto result = solver->solve();

        if(result==10) { // Satisfiable
            return 1;
        }
        if(result==20) { // Unsatisfiable
            return 0;
        }
        if(result==0) { // Limit
            return -1;
        }

        assert(false);
        return 0;
    }


    template<class T>
    int solveWithTimeoutAndLimit(const T &assumption, double timeout_sec, int confBudget, int except=0) {
        solver->reset_assumptions();

        for (int lit : assumption) {
            if (lit == except)
                continue;
            solver->assume(lit);
        }

        solver->limit("conflicts", confBudget);
        solver->setTimeout(timeout_sec);

        auto result = solver->solve();

        if(result==10) { // Satisfiable
            return 1;
        }
        if(result==20) { // Unsatisfiable
            return 0;
        }
        if(result==0) { // Limit
            return -1;
        }

        assert(false);
        return 0;
    }

    template<class T>
    std::vector<int> getConflict(const T &assumptions) {
        std::vector<int> conflicts;
        for (int lit : assumptions) {
            if (solver->failed(lit)) {
                conflicts.push_back(lit);
            }
        }
        return conflicts;
    }

    bool propagate(const std::vector<int> &assum, std::vector<int> &result) {
        return solver->find_up_implicants(assum, result);
    }
};



