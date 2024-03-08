#ifndef EVALMAXSAT_SLK178903R_H
#define EVALMAXSAT_SLK178903R_H


#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <queue>


#include "utile.h"
#include "Chrono.h"
#include "coutUtil.h"
#include "cadicalinterface.h"
#include "cardincremental.h"
#include "rand.h"
#include "mcqd.h"


using namespace MaLib;



class LocalOptimizer2 {
    Solver_cadical *solver;
    WeightVector poids;
    t_weight initialWeight;

public:
    LocalOptimizer2(Solver_cadical *solver, const WeightVector &poids, const t_weight &initialWeight) :
        solver(solver), poids(poids), initialWeight(initialWeight) {

    }

    t_weight calculateCostAssign(const std::vector<bool> & solution) {
        t_weight coutSolution = 0;
        for(unsigned int lit=1; lit<poids.size(); lit++) {
            assert( lit < solution.size() );
            if( poids[lit] > 0 ) {
                if( solution[lit] == 0 ) {
                    coutSolution += poids[lit];
                }
            } else if ( poids[lit] < 0 ) {
                if( solution[lit] == 1 ) {
                    coutSolution += -poids[lit];
                }
            }
        }

        return coutSolution + initialWeight;
    }

    t_weight optimize(std::vector<bool> & solution, double timeout_sec)  {
        auto timeout = MaLib::TimeOut(timeout_sec);
        MonPrint("c optimize for ", timeout_sec, " sec");

        std::vector< std::tuple<t_weight, int> > softVarFalsified;
        std::vector< int > assumSatisfied;
        for(unsigned int lit=1; lit<poids.size(); lit++) {
            assert( lit < solution.size() );
            if( poids[lit] > 0 ) {
                assert(lit < solution.size() );
                if( solution[lit] == 0 ) {
                    softVarFalsified.push_back( {poids[lit], lit} );
                } else {
                    assumSatisfied.push_back(lit);
                }
            } else if( poids[lit] < 0 ) {
                assert(lit < solution.size() );
                if( solution[lit] == 1 ) {
                    softVarFalsified.push_back( {poids[-lit], -lit} );
                } else {
                    assumSatisfied.push_back(-lit);
                }
            }
        }

        bool modify = false;
        for(int id=softVarFalsified.size()-1; id >= 0; id--) {
            assumSatisfied.push_back( std::get<1>(softVarFalsified[id]) );
            if( solver->solveLimited(assumSatisfied, timeout_sec / (double)(softVarFalsified.size()+1) ) != 1 ) {
                assumSatisfied.pop_back();
            } else {
                modify = true;
            }
        }

        if(modify) {
            auto tmp = solver->solve(assumSatisfied);
            assert(tmp == 1);
            solution = solver->getSolution();
        }

        return calculateCostAssign(solution);
    }

};

template<class SAT_SOLVER=Solver_cadical>
class EvalMaxSAT {

    ///////////////////////////
    /// Representation of the MaxSAT formula
    ///
        SAT_SOLVER *solver = nullptr;
        WeightVector _poids; // _poids[lit] = weight of lit
        std::map<t_weight, std::set<int>> _mapWeight2Assum; // _mapWeight2Assum[weight] = set of literals with this weight
        t_weight cost = 0;

        struct LitCard {
            std::shared_ptr<CardIncremental_Lazy<EvalMaxSAT<SAT_SOLVER>> > card;
            int atMost;
            t_weight initialWeight;

            LitCard(std::shared_ptr<CardIncremental_Lazy<EvalMaxSAT<SAT_SOLVER>>> card, int atMost, t_weight initialWeight) : card(card), atMost(atMost), initialWeight(initialWeight) {
                assert( card != nullptr );
                assert(atMost > 0);
                assert(initialWeight > 0);
            }

            int getLit() const {
                return card->atMost(atMost);
            }
        };
        std::vector< std::optional<LitCard> > _mapAssum2Card; // _mapAssum2Card[lit] = card associated to lit

        std::deque< std::tuple< std::vector<int>, t_weight> > _cardToAdd;   // cardinality to add
        std::vector<int> _litToRelax; // Lit to relax
    ///
    //////////////////////////

public:
    ///////////////////////////
    /// Best solution found so far
    ///
        std::vector<bool> solution;
        t_weight solutionCost = std::numeric_limits<t_weight>::max();
    ///
    //////////////////////////
private:
    ///////////////////////////
    /// Hyperparameters
    ///
        double _initialCoef = 10;
        double _averageCoef = 1.66;
        double _base = 400;
        double _minimalRefTime = 5;
        double _maximalRefTime = 5*60;
        TimeOut totalSolveTimeout = TimeOut(0.9 * 3600 );

        bool _delayStrategy = true;
        bool _multiSolveStrategy = true;
        bool _UBStrategy = true;
    ///
    //////////////////////////////

    double getTimeRefCoef() {
        if(totalSolveTimeout.getCoefPast() >= 1)
            return 0;
        return _initialCoef * pow(_base, -totalSolveTimeout.getCoefPast());
    }

public:



    ///////////////////////////
    /// Setter
    ///
        void setCoef(double initialCoef, double coefOnRefTime) {
            auto W = [](double x){
                // Approximation de la fonction W de Lambert par la série de Taylor
                double result = x;
                x *= x;
                result += -x;
                x *= x;
                result += 3*x/2.0;
                x *= x;
                result += -8*x/3.0;
                x *= x;
                result += 125*x/24.0;
                return result;
            };

            _initialCoef = initialCoef;
            _averageCoef = coefOnRefTime;
            _base = (-initialCoef / ( coefOnRefTime * W( (-initialCoef * std::exp(-initialCoef/coefOnRefTime)) / coefOnRefTime ) ));
        }
        void setTargetComputationTime(double targetComputationTime) {
            totalSolveTimeout = TimeOut( targetComputationTime * 0.9 );
        }
        void setBoundRefTime(double minimalRefTime, double maximalRefTime) {
            _minimalRefTime = minimalRefTime;
            _maximalRefTime = maximalRefTime;
        }
        void unactivateDelayStrategy() {
            _delayStrategy = false;
        }
        void unactivateMultiSolveStrategy() {
            _multiSolveStrategy = false;
        }
        void unactivateUBStrategy() {
            _UBStrategy = false;
        }
    ///
    ////////////////////////


    public:
    EvalMaxSAT() : solver(new SAT_SOLVER) {
        _poids.push_back(0);          // Fake lit with id=0
        _mapAssum2Card.push_back({});  // Fake lit with id=0
    }

    ~EvalMaxSAT() {
        delete solver;
    }
public:
    void printInfo() {
        std::cout << "c PARAMETRE INFORMATION: " << std::endl;
        std::cout << "c Stop unsat improving after " << totalSolveTimeout.getVal() << " sec" << std::endl;
        std::cout << "c Minimal ref time = " << _minimalRefTime << " sec" << std::endl;
        std::cout << "c Initial coef = " << _initialCoef << std::endl;
        std::cout << "c Average coef = " << _averageCoef << std::endl;
    }


    bool isWeighted() {
        return _mapWeight2Assum.size() > 1;
    }

    int newVar(bool decisionVar=true) {
        _poids.push_back(0);
        _mapAssum2Card.push_back({});

        int var = solver->newVar(decisionVar);

        assert(var == _poids.size()-1);

        return var;
    }

    int newSoftVar(bool value, long long weight) {
        if(weight < 0) {
            value = !value;
            weight = -weight;
        }

        _poids.push_back(weight * (((int)value)*2 - 1));
        _mapAssum2Card.push_back({});
        _mapWeight2Assum[weight].insert( (((int)value*2) - 1) * ((int)(_poids.size())-1) );

        int var = solver->newVar();
        assert(var == _poids.size()-1);

        return var;
    }

    int addClause(const std::vector<int> &clause, std::optional<long long> w = {}) {
        if( w.has_value() == false ) { // Hard clause
            if( (clause.size() == 1) && ( _poids[clause[0]] != 0 ) ) {
                if( _poids[clause[0]] < 0 ) {
                    assert( _mapWeight2Assum[ _poids[-clause[0]] ].count( -clause[0] ) );
                    _mapWeight2Assum[ _poids[-clause[0]] ].erase( -clause[0] );
                    cost += _poids[-clause[0]];
                    relax(clause[0]);
                } else {
                    assert( _mapWeight2Assum[ _poids[clause[0]] ].count( clause[0] ) );
                    _mapWeight2Assum[ _poids[clause[0]] ].erase( clause[0] );
                }
                _poids.set(clause[0], 0);
                //relax(clause[0]);
            }

            solver->addClause(clause);
        } else {
            if(w.value() == 0)
                return 0;
            if(clause.size() > 1) { // Soft clause, i.e, "hard" clause with a soft var at the end
                if( w.value() > 0 ) {
                    auto softVar = newSoftVar(true, w.value());
                    std::vector<int> softClause = clause;
                    softClause.push_back( -softVar );
                    addClause(softClause);
                    assert(softVar > 0);
                    return softVar;
                } else {
                    assert(w.value() < 0);
                    auto softVar = newSoftVar(true, -w.value());
                    for(auto lit: clause) {
                        addClause({ -softVar, lit });
                    }
                    assert(softVar > 0);
                    return softVar;
                }
            } else if(clause.size() == 1) { // Special case: unit clause.
                addWeight(clause[0], w.value());
            } else { assert(clause.size() == 0); // Special case: empty soft clause.
                cost += w.value();
            }
        }
        return 0;
    }

    t_weight getCost() {
        return solutionCost;
    }

    bool getValue(int lit) {
        if(abs(lit) > solution.size())
            return false;
        if(lit>0)
            return solution[lit];
        if(lit<0)
            return !solution[-lit];
        assert(false);
        return 0;
    }


private:

    std::set<int> initAssum(t_weight minWeightToConsider=1) {
        std::set<int> assum;

        for(auto itOnMapWeight2Assum = _mapWeight2Assum.rbegin(); itOnMapWeight2Assum != _mapWeight2Assum.rend(); ++itOnMapWeight2Assum) {
            if(itOnMapWeight2Assum->first < minWeightToConsider)
                break;
            for(auto lit: itOnMapWeight2Assum->second) {
                assum.insert(lit);
            }
        }
        return assum;
    }

    bool toOptimize = true;
public:
    void disableOptimize() {
        toOptimize = false;
    }

    bool solve() {
        // TODO: Support incremental solve

        totalSolveTimeout.restart();

        MonPrint("c initial cost = ", cost);
        std::set<int> assum;
        std::set<int> lastSatAssum;

        Chrono chronoLastSolve;
        Chrono chronoLastOptimize;

        adapt_am1_exact();
        adapt_am1_FastHeuristicV7();

        if(cost >= solutionCost) {
            return true;
        }

        if(_mapWeight2Assum.size() == 0) {
            if(solver->solve()) {
                solutionCost = cost;
                solution = solver->getSolution();
                return true;
            }
            
            return false;
        }

        if(harden(assum)) {
            assert(_mapWeight2Assum.size());
            if(adapt_am1_VeryFastHeuristic()) {
                if(cost >= solutionCost) {
                    return true;
                }
            }
        }

        ///////////////////
        /// For harden via optimize
        ///
            assert(_cardToAdd.size() == 0);
            assert(_litToRelax.size() == 0);
            assert( [&](){
                for(auto e: _mapAssum2Card) {
                    if(e.has_value())
                        return false;
                }
                return true;
            }() );

            LocalOptimizer2 LO(solver, _poids, cost);
        //
        /////////////////



        // Initialize assumptions
        auto minWeightToConsider = chooseNextMinWeight( _mapWeight2Assum.rbegin()->first + 1 );
        assum = initAssum(minWeightToConsider);

        //std::cout << "o " << solutionCost << std::endl;

        int resultLastSolve;
        for(;;) {
            MonPrint("Full SAT...");
            assert( _cardToAdd.size() == 0 );
            assert( _litToRelax.size() == 0 );

            if(cost >= solutionCost) {
                return true;
            }

            chronoLastSolve.tic();
            {

                 assert( assum == initAssum(minWeightToConsider) );

                //Chrono2 log("solve ");
                resultLastSolve = solver->solve(assum);
                MonPrint("resultLastSolve = ", resultLastSolve);
                if(resultLastSolve == 1) {
                    lastSatAssum = assum;
                }
            }
            chronoLastSolve.pause(true);
            if(resultLastSolve) { // SAT
                if(minWeightToConsider == 1) {
                    solutionCost = cost;
                    solution = solver->getSolution();
                    return true; // Solution found
                }

                minWeightToConsider = chooseNextMinWeight(minWeightToConsider);
                assum = initAssum(minWeightToConsider);

            } else { // UNSAT
                for(;;) {
                    if(resultLastSolve == 0) { // UNSAT
                        double maxLastSolveOr1 = std::min<double>(_minimalRefTime + chronoLastSolve.tacSec(), _maximalRefTime);

                        auto conflict = solver->getConflict(assum);
                        MonPrint("conflict size = ", conflict.size(), " trouvé en ", chronoLastSolve.tacSec(), "sec, donc maxLastSolveOr1 = ", maxLastSolveOr1);

                        if(conflict.size() == 0) {
                            MonPrint("Fin par coupure !");
                            assert(solver->solve() == 0);
                            return solutionCost != std::numeric_limits<t_weight>::max();
                        }

                        // 1. find better core : seconds solve
                        unsigned int nbSolve=0;
                        unsigned int lastImprove = 0;
                        double bestP = std::numeric_limits<double>::max();
                        if(_multiSolveStrategy && (conflict.size() > 3)) {
                            Chrono C;
                            while( ( nbSolve < 3*lastImprove ) || (nbSolve < 20) || ( (C.tacSec() < 0.1*maxLastSolveOr1*getTimeRefCoef()) && (nbSolve < 1000) ) ) {
                                nbSolve++;

                                auto tmp = solver->getConflict(assum);
                                auto p = oneMinimize(tmp, maxLastSolveOr1 * getTimeRefCoef() * 0.1, 10, true);

                                if(p<bestP) {
                                    bestP = p;
                                    conflict = tmp;
                                    MonPrint("Improve conflict at the ", nbSolve, "th solve = ", conflict.size(),  " (p = ", p , ")");
                                    if(conflict.size() <= 2) {
                                        break;
                                    }
                                    lastImprove = nbSolve;
                                }

                                if(C.tacSec() >= maxLastSolveOr1*getTimeRefCoef() ) {
                                    MonPrint("Stop second solve after ", nbSolve, "th itteration. (nb=",conflict.size(),"). (", C.tacSec(), " > ", maxLastSolveOr1, " * ", getTimeRefCoef(), ")");
                                    break;
                                }

                                std::vector<int> vAssum(assum.begin(), assum.end());
                                MaLib::MonRand::shuffle(vAssum);
                                auto res = solver->solveWithTimeout(vAssum, maxLastSolveOr1 * getTimeRefCoef() - C.tacSec()  );
                                if(res==-1) {
                                    MonPrint("Stop second solve because solveWithTimeout. (nb=",conflict.size(),").");
                                    break;
                                }
                            }
                            MonPrint("Stop second solve after ", nbSolve, " itteration in ", C.tacSec(), " sec. (nb=",conflict.size(),").");
                        }

                        // 2. minimize core
                        if(conflict.size() > 1) {
                            oneMinimize(conflict, maxLastSolveOr1, 1000);
                        }

                        if(conflict.size() == 0) {
                            return solutionCost != std::numeric_limits<t_weight>::max();
                        }

                        // 3. replace assum by card
                        long long minWeight = std::numeric_limits<long long>::max();
                        for(auto lit: conflict) {
                            assert(_poids[lit] > 0);
                            if( _poids[lit] < minWeight ) {
                                minWeight = _poids[lit];
                            }
                        }
                        assert(minWeight>0);
                        for(auto lit: conflict) {
                            _mapWeight2Assum[_poids[lit]].erase( lit );
                            _poids.add(lit, -minWeight);
                            if(_poids[lit] == 0) {
                                if(_mapAssum2Card[ abs(lit) ].has_value()) {
                                    _litToRelax.push_back(lit);
                                }
                            } else {
                                _mapWeight2Assum[_poids[lit]].insert( lit );
                            }
                            assert(_poids[lit] >= 0);

                            if(_poids[lit] < minWeightToConsider) {
                                assum.erase(lit);
                            }
                        }

                        for(auto& l: conflict) {
                            l *= -1;
                        }
                        _cardToAdd.push_back( {conflict, minWeight} );
                        MonPrint("cost = ", cost, " + ", minWeight);
                        cost += minWeight;
                        if(cost == solutionCost) {
                            MonPrint("c UB == LB");
                            return true;
                        }
                        MonPrint(_mapWeight2Assum.rbegin()->first, " >= ", solutionCost, " - ", cost, " (", solutionCost - cost, ") ?");
                        if( _mapWeight2Assum.rbegin()->first >= solutionCost - cost) {
                            MonPrint("c Déchanchement de Haden !");
                            if(harden(assum)) {
                                assert(_mapWeight2Assum.size());
                                if(adapt_am1_VeryFastHeuristic()) {
                                    assum = initAssum(minWeightToConsider);
                                }
                            }
                        }
                    } else if(resultLastSolve == 1) { // SAT
                        if(_litToRelax.size()) {
                            for(auto lit: _litToRelax) {
                                auto newLit = relax(lit);
                                if(newLit.has_value()) {
                                    if( _poids[newLit.value()] >= minWeightToConsider ) {
                                        assum.insert(newLit.value());
                                    }
                                }
                            }
                            _litToRelax.clear();
                            assert( assum == initAssum(minWeightToConsider) );
                        } else {
                            ///////////////////
                            /// For harden via optimize
                            ///
                                if(_UBStrategy && toOptimize) {
                                    auto curSolution = solver->getSolution();
                                    auto curCost = LO.optimize( curSolution, std::min(0.1 * chronoLastOptimize.tacSec(), 60.0) );

                                    if(curCost < solutionCost) {
                                        std::cout << "o " << curCost << std::endl;
                                        solutionCost = curCost;
                                        solution = curSolution;

                                        if(harden(assum)) {
                                            assert(_mapWeight2Assum.size());

                                            if(adapt_am1_VeryFastHeuristic()) {
                                                assum = initAssum(minWeightToConsider);
                                            }
                                        }
                                    }
                                    chronoLastOptimize.tic();
                                }
                            ///
                            ////////////////

                            while(_cardToAdd.size()) {
                                std::shared_ptr<CardIncremental_Lazy<EvalMaxSAT<SAT_SOLVER>>> card = std::make_shared<CardIncremental_Lazy<EvalMaxSAT<SAT_SOLVER>>>(this, std::get<0>(_cardToAdd.front()), 1);
                                int newAssumForCard = card->atMost(1);
                                if(newAssumForCard != 0) {
                                    assert( _poids[newAssumForCard] == 0 );

                                    _poids.set(newAssumForCard, std::get<1>(_cardToAdd.front()));
                                    _mapWeight2Assum[ std::get<1>(_cardToAdd.front()) ].insert(newAssumForCard);
                                    _mapAssum2Card[ abs(newAssumForCard) ] = LitCard(card, 1, std::get<1>(_cardToAdd.front()));

                                    if( _poids[newAssumForCard] >= minWeightToConsider ) {
                                        assum.insert(newAssumForCard);
                                    }
                                }
                                _cardToAdd.pop_front();
                            }

                            break;
                        }
                    } else { // TIMEOUT
                        minWeightToConsider=1;
                        assum = initAssum(minWeightToConsider);
                        break;
                    }

                    if(_delayStrategy == false) {
                        break;
                    }

                    MonPrint("Limited SAT...");

                    chronoLastSolve.tic();
                    {
                        if(_mapWeight2Assum.size() <= 1) {
                            resultLastSolve = solver->solveWithTimeout(assum, 60);
                        } else {
                            resultLastSolve = solver->solve(assum);
                        }
                        //resultLastSolve = solver->solveLimited(assum, 10000);
                        //resultLastSolve = solver->solve(assum);
                        //resultLastSolve = solver->solveWithTimeout(assum, 60);

                        MonPrint("resultLastSolve = ", resultLastSolve);

                        if(resultLastSolve == 1) {
                            lastSatAssum = assum;
                        }
                    }
                    chronoLastSolve.pause(true);
                }

                for(auto lit: _litToRelax) {
                    auto newLit = relax(lit);
                    if(newLit.has_value()) {
                        if( _poids[newLit.value()] >= minWeightToConsider ) {
                            assum.insert(newLit.value());
                        }
                    }
                }
                _litToRelax.clear();

                for(auto c: _cardToAdd) {
                    std::shared_ptr<CardIncremental_Lazy<EvalMaxSAT<SAT_SOLVER>>> card = std::make_shared<CardIncremental_Lazy<EvalMaxSAT<SAT_SOLVER>>>(this, std::get<0>(c), 1);
                    int newAssumForCard = card->atMost(1);
                    if(newAssumForCard != 0) {
                        assert( _poids[newAssumForCard] == 0 );

                        _poids.set(newAssumForCard, std::get<1>(c));
                        _mapWeight2Assum[ std::get<1>(c) ].insert(newAssumForCard);
                        _mapAssum2Card[ abs(newAssumForCard) ] = LitCard(card, 1, std::get<1>(c));

                        if( _poids[newAssumForCard] >= minWeightToConsider ) {
                            assum.insert(newAssumForCard);
                        }
                    }
                }
                _cardToAdd.clear();
            }
        }

        assert(false);
        return true;
    }


    private:

    int harden(std::set<int> &assum) {
        if(_mapWeight2Assum.size() == 0)
            return 0;
        if(_mapWeight2Assum.size()==1) {
            if(_mapWeight2Assum.begin()->first == 1) {
                return 0;
            }
        }

        int nbHarden = 0;
        t_weight maxCostLit = solutionCost - cost;


        while( _mapWeight2Assum.rbegin()->first >= maxCostLit ) {
            assert(_mapWeight2Assum.size());

            auto lits = _mapWeight2Assum.rbegin()->second;

            for(auto lit: lits) {
                assert( _poids[lit] >= maxCostLit );
                addClause({lit});
                assum.erase(lit);
                nbHarden++;
            }

            if(_mapWeight2Assum.size() == 1) {
                break;
            }

            if(_mapWeight2Assum.rbegin()->second.size() == 0) {
                _mapWeight2Assum.erase(prev(_mapWeight2Assum.end()));
            }
        }

        MonPrint("c NUMBER HARDEN : ", nbHarden);

        return nbHarden;
    }


    double oneMinimize(std::vector<int>& conflict, double referenceTimeSec, unsigned int B=10, bool sort=true) {
        Chrono C;
        double minimize=0;
        double noMinimize=0;
        unsigned int nbRemoved=0;

        if(sort) {
            //if(isWeighted()) {
                std::sort(conflict.begin(), conflict.end(), [&](int litA, int litB){

                    assert(_poids[litA] >= 0);
                    assert(_poids[litB] >= 0);

                    if(_poids[ litA ] > _poids[ litB ])
                        return true;
                    if(_poids[ litA ] < _poids[ litB ]) {
                        return false;
                    }
                    return abs(litA) < abs(litB);
                });
            //}
        }

        if(conflict.size()==0) {
            return 0;
        }
        while( solver->solveLimited(conflict, 10*B, conflict.back()) == 0 ) {
            conflict.pop_back();
            minimize++;
            nbRemoved++;
            if(conflict.size()==0) {
                return 0;
            }

            if((C.tacSec() > referenceTimeSec*getTimeRefCoef())) {
                MonPrint("\t\tbreak minimize");
                return conflict.size() / (double)_poids[ conflict.back() ];
            }
        }

        t_weight weight = _poids[conflict.back()];
        assert(weight>0);
        //MaLib::MonRand::shuffle(conflict.begin(), conflict.end()-1);

        for(int i=conflict.size()-2; i>=0; i--) {
            switch(solver->solveLimited(conflict, B, conflict[i])) {
            case -1: // UNKNOW
                [[fallthrough]];
            case 1: // SAT
            {
                noMinimize++;
                break;
            }

            case 0: // UNSAT
                conflict[i] = conflict.back();
                conflict.pop_back();
                minimize++;
                nbRemoved++;
                break;

            default:
                assert(false);
            }

            if( C.tacSec() > referenceTimeSec*getTimeRefCoef() ) {
                break;
            }
        }

        return conflict.size() / (double)weight;
    }


    //////////////////////////////
    /// For extractAM
    ///
        void extractAM() {
            adapt_am1_exact();
            adapt_am1_FastHeuristicV7();
        }

        bool adapt_am1_exact() {
            Chrono chrono;
            unsigned int nbCliqueFound=0;
            std::vector<int> assumption;

            for(auto & [w, lits]: _mapWeight2Assum) {
                assert(w != 0);
                for(auto lit: lits) {
                    assert( _poids[lit] > 0 );
                    assumption.push_back(lit);
                }
            }

            if(assumption.size() > 30000) { // hyper paramétre
                MonPrint("skip");
                return false;
            }

            MonPrint("Create graph for searching clique...");
            unsigned int size = assumption.size();
            bool **conn = new bool*[size];
            for(unsigned int i=0; i<size; i++) {
                conn[i] = new bool[size];
                for(unsigned int x=0; x<size; x++)
                    conn[i][x] = false;
            }

            MonPrint("Create link in graph...");
            for(unsigned int i=0; i<size; ) {
                int lit1 = assumption[i];

                std::vector<int> prop;
                // If literal in assumptions has a value that is resolvable, get array of all the other literals that must have
                // a certain value in consequence, then link said literal to the opposite value of these other literals in graph

                if(solver->propagate(std::vector<int>({lit1}), prop)) {
                    for(int lit2: prop) {
                        for(unsigned int j=0; j<size; j++) {
                            if(j==i)
                                continue;
                            if(assumption[j] == -lit2) {
                                conn[i][j] = true;
                                conn[j][i] = true;
                            }
                        }
                    }
                    i++;
                } else { // No solution - Remove literal from the assumptions and add its opposite as a clause
                    addClause({-lit1});

                    assumption[i] = assumption.back();
                    assumption.pop_back();

                    for(unsigned int x=0; x<size; x++) {
                        conn[i][x] = false;
                        conn[x][i] = false;
                    }

                    size--;
                }
            }

            if(size == 0) {
                for(unsigned int i=0; i<size; i++) {
                    delete [] conn[i];
                }
                delete [] conn;
                return true;
            }

            std::vector<bool> active(size, true);
            for(;;) {
                int *qmax;
                int qsize=0;
                Maxclique md(conn, size, 0.025);
                md.mcqdyn(qmax, qsize, 100000);

                if(qsize <= 2) { // Hyperparametre: Taille minimal a laquelle arreter la methode exact
                    for(unsigned int i=0; i<size; i++) {
                        delete [] conn[i];
                    }
                    delete [] conn;
                    delete [] qmax;

                    MonPrint(nbCliqueFound, " cliques found in ", (chrono.tacSec()), "sec.");
                    return true;
                }
                nbCliqueFound++;

                {
                    //int newI=qmax[0];
                    std::vector<int> clause;

                    for (unsigned int i = 0; i < qsize; i++) {
                        int lit = assumption[qmax[i]];
                        active[qmax[i]] = false;
                        clause.push_back(lit);

                        for(unsigned int x=0; x<size; x++) {
                            conn[qmax[i]][x] = false;
                            conn[x][qmax[i]] = false;
                        }
                    }
                    auto newAssum = processAtMostOne(clause);
                    assert(qsize >= newAssum.size());

                    for(unsigned int j=0; j<newAssum.size() ; j++) {
                        assumption[ qmax[j] ] = newAssum[j];
                        active[ qmax[j] ] = true;

                        std::vector<int> prop;
                        if(solver->propagate({newAssum[j]}, prop)) {
                            for(int lit2: prop) {
                                for(unsigned int k=0; k<size; k++) {
                                    if(active[k]) {
                                        if(assumption[k] == -lit2) {
                                            conn[qmax[j]][k] = true;
                                            conn[k][qmax[j]] = true;
                                        }
                                    }
                                }
                            }
                         } else {
                            assert(solver->solve(std::vector<int>({newAssum[j]})) == false);
                            addClause({-newAssum[j]});
                         }
                    }
                }

                delete [] qmax;
            }

            assert(false);
        }

        // Harden soft vars in passed clique to then unrelax them via a new cardinality constraint
        std::vector<int> processAtMostOne(std::vector<int> clause) {
            std::vector<int> newAssum;

            while(clause.size() > 1) {

                assert([&](){
                    for(unsigned int i=0; i<clause.size(); i++) {
                        for(unsigned int j=i+1; j<clause.size(); j++) {
                            assert(solver->solve(std::vector<int>({clause[i], clause[j]})) == 0 );
                        }
                    }
                    return true;
                }());

                auto saveClause = clause;
                auto w = _poids[ clause[0] ];
                assert(w > 0);

                for(unsigned int i=1; i<clause.size(); i++) {
                    if( w > _poids[ clause[i] ] ) {
                        w = _poids[ clause[i] ];
                    }
                }
                assert(w > 0);

                for(unsigned int i=0; i<clause.size(); ) {
                    assert( _poids[clause[i]] > 0 );
                    assert( _mapWeight2Assum[ _poids[ clause[i] ] ].count( clause[i] ) );
                    _mapWeight2Assum[ _poids[ clause[i] ] ].erase( clause[i] );

                    _poids.add( clause[i], -w );

                    assert( _poids[ clause[i] ] >= 0 );
                    if( _poids[ clause[i] ] == 0 ) {
                        relax( clause[i] );
                        clause[i] = clause.back();
                        clause.pop_back();
                    } else {
                        _mapWeight2Assum[ _poids[ clause[i] ] ].insert( clause[i] );
                        i++;
                    }
                }
                MonPrint("AM1: cost = ", cost, " + ", w * (t_weight)(saveClause.size()-1));
                cost += w * (t_weight)(saveClause.size()-1);

                assert(saveClause.size() > 1);
                newAssum.push_back( addClause(saveClause, w) );
                assert(newAssum.back() != 0);
                assert( _poids[ newAssum.back() ] > 0 );
            }

            if( clause.size() ) {
                newAssum.push_back(clause[0]);
            }
            return newAssum;
        }

        void reduceCliqueV2(std::list<int> & clique) {
            if(_mapWeight2Assum.size() > 1) {
                clique.sort([&](int litA, int litB){
                    assert( _poids[ -litA ] > 0 );
                    assert( _poids[ -litB ] > 0 );

                    return _poids[ -litA ] < _poids[ -litB ];
                });
            }
            for(auto posImpliquant = clique.begin() ; posImpliquant != clique.end() ; ++posImpliquant) {
                auto posImpliquant2 = posImpliquant;
                for(++posImpliquant2 ; posImpliquant2 != clique.end() ; ) {
                    if(solver->solveLimited(std::vector<int>({-(*posImpliquant), -(*posImpliquant2)}), 10000) != 0) { // solve != UNSAT
                        posImpliquant2 = clique.erase(posImpliquant2);
                    } else {
                        ++posImpliquant2;
                    }
                }
            }
        }

        unsigned int adapt_am1_VeryFastHeuristic() {
            std::vector<int> prop;
            unsigned int nbCliqueFound=0;

            for(int VAR = 1; VAR<_poids.size(); VAR++) {
                if(_poids[VAR] == 0)
                    continue;
                assert(_poids[VAR] != 0);

                int LIT = _poids[VAR]>0?VAR:-VAR;
                prop.clear();

                if(solver->propagate({LIT}, prop)) {
                    if(prop.size() == 0)
                        continue;

                    for(auto litProp: prop) {
                        if(_poids[litProp] < 0) {
                            assert(solver->solve(std::vector<int>({-litProp, LIT})) == false);
                            processAtMostOne( {-litProp, LIT} );
                            nbCliqueFound++;
                            if(_poids[VAR] == 0)
                                break;
                        }
                    }
                } else {
                    nbCliqueFound++;
                    addClause({-LIT});
                }
            }

            return nbCliqueFound;
        }

        unsigned int adapt_am1_FastHeuristicV7() {
            MonPrint("adapt_am1_FastHeuristic : (_weight.size() = ", _poids.size(), " )");

            Chrono chrono;
            std::vector<int> prop;
            unsigned int nbCliqueFound=0;

            for(int VAR = 1; VAR<_poids.size(); VAR++) {
                if(_poids[VAR] == 0)
                    continue;

                assert(_poids[VAR] != 0);

                int LIT = _poids[VAR]>0?VAR:-VAR;
                prop.clear();
                if(solver->propagate({LIT}, prop)) {
                    if(prop.size() == 0)
                        continue;

                    std::list<int> clique;
                    for(auto litProp: prop) {
                        if(_poids[litProp] < 0) {
                            clique.push_back(litProp);
                            assert(solver->solve(std::vector<int>({-litProp, LIT})) == false);
                        }
                    }

                    if(clique.size() == 0)
                        continue;

                    reduceCliqueV2(clique); // retirer des elements pour que clique soit une clique

                    clique.push_back(-LIT);

                    if(clique.size() >= 2) {
                        nbCliqueFound++;

                        std::vector<int> clause;
                        for(auto lit: clique)
                            clause.push_back(-lit);

                        processAtMostOne(clause);
                    }
                } else {
                    nbCliqueFound++;
                    addClause({-LIT});
                }
            }

            MonPrint(nbCliqueFound, " cliques found in ", chrono);
            return nbCliqueFound;
        }
    ///
    /// End for extractAM
    ///////////////////////


public:
    void addWeight(int lit, long long weight) {
        assert(lit != 0);
        assert(weight != 0 && "Not an error, but it should not happen");
        if(weight < 0) {
            weight = -weight;
            lit = -lit;
        }

        while(abs(lit) >= _poids.size()) {
            newVar();
        }

        if( _poids[lit] == 0 ) {
            _poids.set(lit, weight);
            _mapWeight2Assum[weight].insert(lit);
        } else {
            if( _poids[lit] > 0 ) {
                assert( _mapWeight2Assum[_poids[lit]].count( lit ) );
                _mapWeight2Assum[_poids[lit]].erase( lit );
                _poids.add(lit, weight);
                assert( _poids[lit] < 0 ? !_mapAssum2Card[lit].has_value() : true ); // If -lit becomes a soft var, it should not be a cardinality
            } else { // if( _poids[lit] < 0 )
                assert( _mapWeight2Assum[_poids[-lit]].count( -lit ) );
                _mapWeight2Assum[_poids[-lit]].erase( -lit );

                cost += std::min(weight, _poids[-lit]);
                _poids.add(lit, weight);
                assert( _poids[-lit] > 0 ? !_mapAssum2Card[-lit].has_value() : true ); // If lit becomes a soft var, it should not be a cardinality
            }

            if(_poids[lit] != 0) {
                if(_poids[lit] > 0) {
                    _mapWeight2Assum[_poids[lit]].insert( lit );
                } else {
                    _mapWeight2Assum[-_poids[lit]].insert( -lit );
                }
            } else {
                relax(lit);
            }
        }
    }
private:


    // If a soft variable is not soft anymore, we have to check if this variable is a cardinality, in which case, we have to relax the cardinality.
    std::optional<int> relax(int lit) {
        assert(lit != 0);
        std::optional<int> newSoftVar;

        unsigned int var = abs(lit);

        if(_mapAssum2Card[var].has_value()) { // If there is a cardinality constraint associated to this soft var
            int forCard = _mapAssum2Card[ var ]->card->atMost( _mapAssum2Card[ var ]->atMost + 1 );

            if(forCard != 0) {
                if( _mapAssum2Card[abs(forCard)].has_value() == false ) {
                    _mapAssum2Card[abs(forCard)] = LitCard(_mapAssum2Card[var]->card,  _mapAssum2Card[var]->atMost + 1,  _mapAssum2Card[var]->initialWeight);
                    assert( forCard == _mapAssum2Card[abs(forCard)]->getLit() );

                    _poids.set(forCard, _mapAssum2Card[abs(forCard)]->initialWeight);
                    _mapWeight2Assum[_poids[forCard]].insert( forCard );

                    newSoftVar = forCard;
                }
            }

            if(_poids[lit] == 0) {
                _mapAssum2Card[var].reset();
            }
        }
        return newSoftVar;
    }

    t_weight chooseNextMinWeight(t_weight minWeightToConsider=std::numeric_limits<t_weight>::max()) {
        auto previousWeight = minWeightToConsider;

        if(_mapWeight2Assum.size() <= 1) {
            return 1;
        }

        for(;;) {
            auto it = _mapWeight2Assum.lower_bound(minWeightToConsider);
            if(it == _mapWeight2Assum.begin()) {
                return 1;
            }
            --it;

            if(it == _mapWeight2Assum.end()) {
                assert(!"possible ?");
                return 1;
            }

            if( it->second.size() == 0 ) {
                _mapWeight2Assum.erase(it);

                if(_mapWeight2Assum.size() <= 1) {
                    return 1;
                }
            } else {
               auto it2 =it;
               it2--;
               if(it2 == _mapWeight2Assum.end()) {
                   MonPrint("minWeightToConsider == ", 1);
                   return 1;
               }

               /*
               if(it2->first < it->first * 0.1 ) {   // hyper paramétre
                   MonPrint("minWeightToConsider apres = ", it->first);
                   return it->first;
               }
               */

               if(it2->first < previousWeight * 0.5) {  // hyper paramétre
                   MonPrint("minWeightToConsider = ", it->first);
                   return it->first;
               }

               minWeightToConsider = it->first;
            }
        }

        assert(false);
        return 1;
    }

public:
    unsigned int nInputVars=0;
    void setNInputVars(unsigned int nInputVars) {
        this->nInputVars=nInputVars;
    }
    unsigned int nVars() {
        return solver->nVars();
    }
};





#endif // EVALMAXSAT_SLK178903R_H

