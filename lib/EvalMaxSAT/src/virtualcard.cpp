
#include "virtualcard.h"

#include "virtualsat.h"

unsigned int VirtualCard::newVar() {
    return solver->newVar(false);
}

void VirtualCard::addClause(const std::vector<int> & clause) {
    solver->addClause(clause);
}

std::ostream& operator<<(std::ostream& os, const VirtualCard& dt) {
    dt.print(os);
    return os;
}

bool VirtualCard::getValue(int lit) {
    return solver->getValue(lit);
}


void VirtualCard::setDecisionVar(unsigned int var, bool value) {
    solver->setDecisionVar(var, value);
}


VirtualCard::~VirtualCard() {

}
