#ifndef IMPLICATIONGRAPH_H
#define IMPLICATIONGRAPH_H

#include <optional>
#include <vector>
#include <memory>
#include <cassert>

#include "virtualcard.h"
#include "coutUtil.h"

class VirtualSAT;

class LazyVariable {

    VirtualSAT *solver;

    std::optional<int> var = {};

    std::vector< std::vector< std::shared_ptr<LazyVariable> > > impliquants;

    unsigned int countUsed = 0;

    LazyVariable( VirtualSAT *solver )
        : solver(solver) {

    }

public:

    static std::shared_ptr<LazyVariable> encapsulate(int variable) {
        assert(variable != 0);
        auto result = std::shared_ptr<LazyVariable>(new LazyVariable(nullptr));
        result->var = variable;
        return result;
    }

    static std::shared_ptr<LazyVariable> newVar(VirtualSAT *solver) {
        return std::shared_ptr<LazyVariable>(new LazyVariable(solver));
    }

    // (\wedge_{v \in vars} v) => this
    void addImpliquant(const std::vector< std::shared_ptr<LazyVariable> > &vars) {
        assert(vars.size() > 0);
        impliquants.push_back( vars );
    }

    int get();

    void stopUse(VirtualSAT *solver);
    void use(VirtualSAT *solver);
};










#endif // IMPLICATIONGRAPH_H
