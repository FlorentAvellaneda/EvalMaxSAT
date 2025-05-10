#pragma once

#include <optional>
#include <vector>
#include <memory>
#include <cassert>



template<class T>
class LazyVariable {

    T *solver;

    std::optional<int> var = {};

    // For a node without 'var'. Lists clauses ; one of them should be satisfied for this variable to be "on" for the unary number.
    std::vector< std::vector< std::shared_ptr<LazyVariable<T>> > > impliquants;



public:

    // The variable will be created when the first call to get() is made.
    LazyVariable(T *solver)
        : solver(solver) {
    }

    // Links the variable to the given one.
    LazyVariable(int variable)
        : solver(nullptr), var(variable) {
        assert(variable != 0);
    }

    // (\wedge_{v \in lazyVars} v) => this
    void addImpliquant(const std::vector< std::shared_ptr<LazyVariable<T>> > &lazyVars) {
        assert(lazyVars.size() > 0);
        impliquants.push_back( lazyVars );
    }

    int get() {
        if(!var) {
            assert(impliquants.size() > 0);

            // If there is only one clause, and only one variable in it, then we can directly link the variable to that one.
            if(impliquants.size() == 1) {
                assert(impliquants[0].size() > 0);
                if(impliquants[0].size() == 1 ) {
                    var = impliquants[0][0]->get();
                    return *var;
                }
            }

            var = solver->newVar(/*false*/);

            /*
             * Add the cardinality constraints to the SatSolver in a recursive manner.
             * Example follows. If we have the vars O1 ... O5 at root, then all the combinations of lits will be added as
             * a clause, with a soft var at the end to make it optional ("soft") :
             * { l1, <softV> }, ..., { l1, l2, l3, <softV> }, ... , { l1, l2, l3, l4, l5, <soft> }
             */
            for(auto &implique: impliquants) {
                std::vector<int> clause;
                for(auto &lazyVar: implique) {
                    int newVar = lazyVar->get();
                    assert( newVar != 0);
                    clause.push_back(-newVar);
                }
                assert(clause.size() > 0);

                clause.push_back(*var);

                solver->addClause( clause );
            }
        }

        return *var;
    }
};






