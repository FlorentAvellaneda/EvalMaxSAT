#ifndef EVALMAXSAT_SLK178903R_H
#define EVALMAXSAT_SLK178903R_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

#include "communicationlist.h"
#include "Chrono.h"
#include "coutUtil.h"
#include "virtualmaxsat.h"
#include "virtualsat.h"
#include "monglucose41.h"
#include "mcqd.h"
#include "coutUtil.h"

using namespace MaLib;


class EvalMaxSAT : public VirtualMAXSAT {
    unsigned int nbMinimizeThread;

    VirtualSAT *solver = nullptr;
    std::vector<VirtualSAT*> solverForMinimize;


    std::vector<unsigned int> _weight;
    std::vector<bool> model;
    std::vector<int> mapAssum2card;
    std::vector< std::tuple<std::shared_ptr<VirtualCard>, int> > save_card; // { {1, -2, 4}, 1 } <==> a v !b v d <= 1

    MaLib::Chrono chronoLastSolve;

    std::atomic<int> cost = 0;
    unsigned int _timeOutFastMinimize=60;
    unsigned int _coefMinimizeTime = 2.0;

    ///// For communication between threads
    MaLib::CommunicationList< std::tuple<std::list<int>, long> > CL_ConflictToMinimize; // {conflict, time_used_by_the_solver}
    MaLib::CommunicationList< int > CL_LitToUnrelax;
    MaLib::CommunicationList< int > CL_LitToRelax;
    MaLib::CommunicationList< std::tuple<std::vector<int>, bool> > CL_CardToAdd;
    std::atomic<unsigned int> numberMinimizeThreadRunning = 4;
    /////



    struct CompLit {
      bool operator() (const int& a, const int& b) const {
          if(abs(a) < abs(b))
              return true;

          if(abs(a) == abs(b))
              return (a>0) && (b<0);

          return false;
      }
    };


    //////////////////////////////
    ////// For extractAM ////////
    ////////////////////////////
    void extractAM() {
        adapt_am1_exact();
        adapt_am1_FastHeuristicV7();
    }

    void reduceCliqueV2(std::list<int> & clique) {
        for(auto posImpliquant = clique.begin() ; posImpliquant != clique.end() ; ++posImpliquant) {
            auto posImpliquant2 = posImpliquant;
            for(++posImpliquant2 ; posImpliquant2 != clique.end() ; ) {
                if(solver->solveLimited(std::vector<int>({-(*posImpliquant), -(*posImpliquant2)}), 10000) != -1) {
                    posImpliquant2 = clique.erase(posImpliquant2);
                } else {
                    ++posImpliquant2;
                }
            }
        }
    }

    bool adapt_am1_FastHeuristicV7() {
        MonPrint("adapt_am1_FastHeuristic");
        Chrono chrono;
        std::vector<int> prop;
        unsigned int nbCliqueFound=0;

        for(unsigned int VAR = 1; VAR<_weight.size(); VAR++) {
            if(_weight[VAR] == 0)
                continue;

            assert(_weight[VAR] > 0);
            int LIT = model[VAR]?static_cast<int>(VAR):-static_cast<int>(VAR);
            prop.clear();
            if(solver->propagate({LIT}, prop)) {
                if(prop.size() == 0)
                    continue;
                assert(*prop.begin() == LIT);
                if(prop.size() == 1)
                    continue;

                std::list<int> clique;
                for(auto litProp: prop) {
                    unsigned int var = static_cast<unsigned int>(abs(litProp));
                    if(isInAssum(-litProp)) {
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

                    processAMk(clause, 1);
                }
            } else {
                addClause({-LIT});
            }
        }

        MonPrint(nbCliqueFound, " cliques found in ", chrono.tac() / 1000, "ms.");
        return true;
    }

    bool adapt_am1_exact() {
        Chrono chrono;
        unsigned int nbCliqueFound=0;
        std::vector<int> assumption;

        for(unsigned int i=1; i<_weight.size(); i++) {
            if(_weight[i] > 0) {
                if(model[i]) {
                    assumption.push_back(static_cast<int>(i));
                } else {
                    assumption.push_back(-static_cast<int>(i));
                }
            }
        }

        MonPrint("Nombre d'assumption: ", assumption.size());

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
            if(solver->propagate({lit1}, prop)) {
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
            } else {
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
                delete conn[i];
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
                    delete conn[i];
                }
                delete [] conn;
                delete qmax;

                MonPrint(nbCliqueFound, " cliques found in ", (chrono.tac() / 1000), "ms.");
                return true;
            }
            nbCliqueFound++;

            {
                int newI=qmax[0];
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
                auto newAssum = processAMk(clause, 1);
                assert(qsize >= newAssum.size());

                for(unsigned int j=0; j<newAssum.size() ; j++) {
                    assumption[ qmax[j] ] = newAssum[j];
                    active[ qmax[j] ] = true;

                    std::vector<int> prop;
                    if(solver->propagate({newAssum[j]}, prop)) {
                        for(int lit2: prop) {
                            for(unsigned int j=0; j<size; j++) {
                                if(active[j]) {
                                    if(assumption[j] == -lit2) {
                                        conn[newI][j] = true;
                                        conn[j][newI] = true;
                                    }
                                }
                            }
                        }
                    } else {
                        assert(!"Possible?");
                        addClause({-newAssum[j]});
                    }
                }
            }

            delete qmax;
        }

        assert(false);
    }

    template<class T>
    std::vector<int> processAMk(const T &clause, unsigned int am) {
        std::vector<int> newAssum;

        assert(am < clause.size());
        assert(am > 0);

        unsigned int k = clause.size()-am;

        int newAssumForCard = 0;

        assert(clause.size() > 1);

        std::vector<int> L;
        for(auto lit: clause) {
            assert(lit!=0);
            unsigned int var = static_cast<unsigned int>(abs(lit));

            L.push_back(-lit);
            _weight[var] = 0;

            if(mapAssum2card[var] != -1) {
                int tmp = mapAssum2card[var];
                assert(tmp >= 0);

                std::get<1>(save_card[tmp])++;
                int forCard = (*std::get<0>(save_card[tmp]) <= std::get<1>(save_card[tmp]));

                if(forCard != 0) {
                    newAssum.push_back(forCard);
                    _weight[abs(forCard)] = 1;
                    mapAssum2card[ abs(forCard) ] = tmp;
                }
            }
        }

        assert(L.size()>1);
        assert(k < L.size());

        cost += k;
        MonPrint("cost = ", cost);

        if(L.size() == 2) { // Optional, just to no add useless card to save_card
            newAssum.push_back( addWeightedClause( {-L[0], -L[1]}, 1 ) );
            return newAssum;
        }

        save_card.push_back( {newCard(L, k), k} );
        newAssumForCard = (*std::get<0>(save_card.back()) <= std::get<1>(save_card.back()));

        if(newAssumForCard != 0) {
            newAssum.push_back(newAssumForCard);
            _weight[abs(newAssumForCard)] = 1;
            mapAssum2card[abs(newAssumForCard)] = save_card.size()-1;
        }

        return newAssum;
    }
    ///////////////////////////




public:
    EvalMaxSAT(unsigned int nbMinimizeThread=4, VirtualSAT *solver =
            //new MonMiniSAT()
            new MonGlucose41()
            //new MonMaple()
            //new MonCaDiCaL()
            //new MonMiniCard()
    ) : nbMinimizeThread(nbMinimizeThread), solver(solver)
    {
        for(unsigned int i=0; i<nbMinimizeThread; i++) {
            solverForMinimize.push_back(new MonGlucose41());
        }

        _weight.push_back(0);           //
        model.push_back(false);         // Fake lit with id=0
        mapAssum2card.push_back(-1);    //
    }

    virtual ~EvalMaxSAT();

    virtual void addClause(const std::vector<int> &vclause) {
        if(vclause.size() == 1) {
            unsigned int var1 = abs(vclause[0]);
            if( _weight[var1] > 0 ) {
                if(model[var1] == (vclause[0] < 0)) {
                    cost += _weight[var1];
                    MonPrint("cost = ", cost);
                }
                _weight[var1] = 0;
            }
        }

        solver->addClause(vclause);
        for(auto s: solverForMinimize) {
            s->addClause(vclause);
        }
    }

    virtual void simplify() {
        assert(!"TODO");
    }

    virtual bool solve(const std::vector<int> &assumption) {
        assert(!"TODO");
    }

    virtual int solveLimited(const std::vector<int> &assumption, int confBudget) {
        assert(!"TODO");
    }

    virtual std::vector<int> getConflict() {
        assert(!"TODO");
    }

    virtual void setDecisionVar(unsigned int var, bool b) {
        solver->setDecisionVar(var, b);
    }

    bool isInAssum(int lit) {
        unsigned int var = static_cast<unsigned int>(abs(lit));
        if( _weight[var] > 0 ) {
            if( model[var] == (lit>0) )
                return true;
        }
        return false;
    }


    private:


    void minimize(VirtualSAT* S, std::list<int> & conflict, long refTime, bool doFastMinimize) {
        std::vector<int> uselessLit;
        std::vector<int> L;
        bool completed=false;
        bool doExhaust=true;
        if(!doFastMinimize) {
            std::set<int> conflictMin(conflict.begin(), conflict.end());
            completed = fullMinimize(S, conflictMin, uselessLit, _coefMinimizeTime*refTime);
            if(completed) {
                doExhaust=false;
            }
            for(auto lit: conflictMin) {
                L.push_back(-lit);
            }
        } else {
            MonPrint("minimize: skip car plus de 100000 ");
            //completed = fastMinimize(S, conflict);

            if(completed) {
                doExhaust=false;
            }

            for(auto lit: conflict) {
                L.push_back(-lit);
            }
        }

        CL_LitToUnrelax.pushAll(uselessLit);

        if(L.size() > 1) {
            CL_CardToAdd.push({L, !completed});
        }

        for(auto lit: L) {
            CL_LitToRelax.push(-lit);
        }

        MonPrint("size conflict after Minimize: ", conflict.size());
    }

    void threadMinimize(unsigned int num, bool fastMinimize) {
        for(;;) {
            auto element = CL_ConflictToMinimize.pop();
            MonPrint("threadMinimize[",num,"]: Run...");

            if(!element) {
                break;
            }

            minimize(solverForMinimize[num], std::get<0>(*element), std::get<1>(*element), fastMinimize);
        }
    }

    void apply_CL_CardToAdd(std::set<int, CompLit> &assumption) {
        while(CL_CardToAdd.size()) {
            auto element = CL_CardToAdd.pop();
            assert(element);

            save_card.push_back( {newCard(std::get<0>(*element)), 1} );
            auto newAssumForCard = (*std::get<0>(save_card.back()) <= std::get<1>(save_card.back()));

            assert(newAssumForCard != 0);

            MonPrint("solveLimited for Exhaust...");
            if(std::get<1>(*element)) {
                while(solver->solveLimited(std::vector<int>({newAssumForCard}), 10000) == -1) {
                    std::get<1>(save_card.back())++;
                    MonPrint("cost = ", cost, " + 1");
                    cost++;
                    newAssumForCard = ((*std::get<0>(save_card.back())) <= std::get<1>(save_card.back()));

                    if(newAssumForCard==0) {
                        break;
                    }
                }
            }
            MonPrint("Exhaust fini!");

            if(newAssumForCard != 0) {
                _weight[abs(newAssumForCard)] = 1;
                assumption.insert( newAssumForCard );
                mapAssum2card[ abs(newAssumForCard) ] = save_card.size()-1;
            }
        }
    }

    void apply_CL_LitToRelax(std::set<int, CompLit> &assumption) {
        while(CL_LitToRelax.size()) {
            int lit = CL_LitToRelax.pop().value_or(0);
            assert(lit != 0);
            unsigned int var = abs(lit);

            _weight[var] = 0;

            if(mapAssum2card[var] != -1) {
                int idCard = mapAssum2card[var];
                assert(idCard >= 0);

                std::get<1>(save_card[idCard])++;
                int forCard = (*std::get<0>(save_card[idCard]) <= std::get<1>(save_card[idCard]));

                if(forCard != 0) {
                    assumption.insert( forCard );
                    _weight[abs(forCard)] = 1;
                    mapAssum2card[ abs(forCard) ] = idCard;
                }
            }
        }
    }


public:

    bool solve() {
        // CONFIG
        unsigned int nbSecondSolveMin = 20;
        unsigned int timeOutForSecondSolve = 60;
        // END CONFIG

        MonPrint("\t\t\tMain Thread: extractAM...");
        extractAM();

        std::set<int, CompLit> assumption;

        for(unsigned int i=1; i<_weight.size(); i++) {
            if(_weight[i] > 0) {
                if(model[i]) {
                    assumption.insert(static_cast<int>(i));
                } else {
                    assumption.insert(-static_cast<int>(i));
                }
            }
        }

        if(assumption.size() > 100000) {
            nbSecondSolveMin = 3;           // hyperparametre
        }

        std::vector<std::thread> vMinimizeThread;
        vMinimizeThread.reserve(nbMinimizeThread);
        for(unsigned int i=0; i<nbMinimizeThread; i++) {
            vMinimizeThread.emplace_back(&EvalMaxSAT::threadMinimize, this, i, assumption.size() > 100000);
        }

        MonPrint("\t\t\tMain Thread: Debut");
        for(;;) {
            assert(CL_ConflictToMinimize.size()==0);
            assert(CL_LitToUnrelax.size()==0);
            assert(CL_LitToRelax.size()==0);
            assert(CL_CardToAdd.size()==0);
            numberMinimizeThreadRunning = nbMinimizeThread;

            //auto s2 = solver->clone();

            bool firstSolve = true;
            for(;;) {
                std::vector<int> forSolve(assumption.begin(), assumption.end());

                chronoLastSolve.tic();
                MonPrint("\t\t\tMain Thread: Solve...");

                bool conflictFound;
                if(firstSolve) {
                    MonPrint("solve(",forSolve.size(),")...");
                    conflictFound = (solver->solve(forSolve) == false);
                } else {
                    MonPrint("solveLimited(",forSolve.size(),")...");
                    conflictFound = (solver->solveLimited(forSolve, 10000) == -1);
                }

                if(!conflictFound) {
                    MonPrint("\t\t\tMain Thread: Solve() is not false!");

                    if(firstSolve) {
                        CL_ConflictToMinimize.close(); // Va impliquer la fin des threads minimize
                        for(auto &t: vMinimizeThread)
                            t.join();

                        return true;
                    }

                    chronoLastSolve.pause(true);
                    MonPrint("\t\t\tMain Thread: CL_ConflictToMinimize.wait(nbMinimizeThread=",nbMinimizeThread,", true)...");
                    do {
                        if(CL_LitToUnrelax.size()) {
                            MonPrint("\t\t\tMain Thread: CL_LitToUnrelax.size() = ", CL_LitToUnrelax.size());
                            break;
                        }
                        numberMinimizeThreadRunning = nbMinimizeThread - CL_ConflictToMinimize.getNumberWaiting();
                        assert(numberMinimizeThreadRunning <= nbMinimizeThread);
                    } while( CL_ConflictToMinimize.wait(nbMinimizeThread, true) < nbMinimizeThread );
                    MonPrint("\t\t\tMain Thread: Fin boucle d'attente");

                    if(CL_LitToUnrelax.size()==0) {
                        MonPrint("\t\t\tMain Thread: CL_LitToUnrelax.size()==0");

                        MonPrint("\t\t\tMain Thread: CL_LitToRelax.size() = ", CL_LitToRelax.size());
                        apply_CL_LitToRelax(assumption);

                        MonPrint("\t\t\tMain Thread: CL_CardToAdd.size() = ", CL_CardToAdd.size());
                        apply_CL_CardToAdd(assumption);


                        break;
                    }
                    MonPrint("\t\t\tMain Thread: CL_LitToUnrelax.size()!=0");
                } else {
                    MonPrint("\t\t\tMain Thread: Solve = false");
                    chronoLastSolve.pause(true);

                    std::vector<int> bestUnminimizedConflict = solver->getConflict();
                    if(bestUnminimizedConflict.size() == 0) {
                        cost = -1;
                        return false;
                    }

                    MonPrint("\t\t\tMain Thread: cost = ", cost, " + 1");
                    cost++;

                    MaLib::Chrono chronoForBreak;
                    unsigned int nbSecondSolve = 0;

                    MonPrint("\t\t\tMain Thread: Second solve...");
                    while((nbSecondSolve < nbSecondSolveMin) || (chronoLastSolve.tac() >= chronoForBreak.tac())) {
                        if(bestUnminimizedConflict.size() == 1)
                            break;
                        nbSecondSolve++;
                        if(chronoForBreak.tacSec() > timeOutForSecondSolve)
                            break;
                        if(nbSecondSolve > 10000)
                            break;

                        std::random_shuffle(forSolve.begin(), forSolve.end());

                        bool res = solver->solve(forSolve);
                        assert(!res);

                        if( bestUnminimizedConflict.size() > solver->sizeConflict() ) {
                            bestUnminimizedConflict = solver->getConflict();
                        }
                    }
                    MonPrint("\t\t\tMain Thread: ", nbSecondSolve, " solves done in ", chronoForBreak.tacSec(), "sec");

                    std::list<int> conflictMin;
                    for(auto lit: bestUnminimizedConflict)
                        conflictMin.push_back(-lit);

                    bool doFullMinimize = true;
                    if((assumption.size() < 100000) && (conflictMin.size() > 1)) {
                        MonPrint("\t\t\tMain Thread: fastMinimize(", conflictMin.size(), ")");
                        doFullMinimize = fastMinimize(solver, conflictMin);
                    }

                    MonPrint("\t\t\tMain Thread: taille final du comflict = ", conflictMin.size());

                    if(conflictMin.size() == 1) {
                        MonPrint("\t\t\tMain Thread: Optimal found, no need to fullMinimize");
                        doFullMinimize = false;
                    }

                    for(auto lit: conflictMin) {
                        assumption.erase(lit);
                    }

                    //assert(s2->solve(conflictMin)==false); // This assert can be false. Why??

                    if(doFullMinimize) {
                        MonPrint("\t\t\tMain Thread: call CL_ConflictToMinimize.push");

                        if(nbMinimizeThread == 0) {
                            minimize(solver, conflictMin, chronoLastSolve.tac(), assumption.size() > 100000);
                        } else {
                            CL_ConflictToMinimize.push({conflictMin, chronoLastSolve.tac()});
                        }

                        firstSolve = false;
                    } else {
                        for(auto lit: conflictMin) {
                            CL_LitToRelax.push(lit);
                        }
                        if(conflictMin.size() > 1) {
                            MonPrint("\t\t\tMain Thread: new card");
                            std::vector<int> L;
                            for(auto lit: conflictMin) {
                                L.push_back(-lit);
                            }
                            CL_CardToAdd.push({L, true});
                        }
                        if(firstSolve) {
                            apply_CL_LitToRelax(assumption);
                            apply_CL_CardToAdd(assumption);
                        }
                    }
                }

                while(CL_LitToUnrelax.size()) {
                    auto element = CL_LitToUnrelax.pop();
                    assert(element);
                    assumption.insert(*element);
                }
            }

            //delete s2;
        }

        return true;
    }


    void setTimeOutFast(unsigned int timeOutFastMinimize) {
        _timeOutFastMinimize = timeOutFastMinimize;
    }

    void setCoefMinimize(unsigned int coefMinimizeTime) {
        _coefMinimizeTime = coefMinimizeTime;
    }


    unsigned int nInputVars=0;
    void setNInputVars(unsigned int nInputVars) {
        this->nInputVars=nInputVars;
    }

    int getCost() {
        return cost;
    }

    bool getValue(unsigned int var) {
        return solver->getValue(var);
    }

    virtual unsigned int nVars() {
        return solver->nVars();
    }

    virtual unsigned int nClauses() {
        return solver->nClauses();
    }

    virtual unsigned int newSoftVar(bool value, bool decisionVar, unsigned int weight) {
        assert(weight==1);
        _weight.push_back(weight);
        mapAssum2card.push_back(-1);
        model.push_back(value);

        unsigned int var = solver->newVar(decisionVar);
        for(auto s: solverForMinimize) {
            unsigned int var2 = s->newVar(decisionVar);
            assert(var == var2);
        }

        assert(var == _weight.size()-1);

        return var;
    }

    virtual unsigned int newVar(bool decisionVar=true) {
        _weight.push_back(0);
        mapAssum2card.push_back(-1);
        model.push_back(false);

        unsigned int var = solver->newVar(decisionVar);
        for(auto s: solverForMinimize) {
            unsigned int var2 = s->newVar(decisionVar);
            assert(var == var2);
        }

        assert(var == _weight.size()-1);

        return var;
    }

    virtual bool isSoft(unsigned int var) {
        return _weight[var] > 0;
    }

    virtual void setVarSoft(unsigned int var, bool value, unsigned int weight) {
        assert(weight==1);
        if(var >= _weight.size()) {
            _weight.resize(var+1, 0);
            mapAssum2card.resize(var+1, -1);
            model.resize(var+1);
        }

        assert(_weight[var] == 0);      // We assum the variable is not already soft
        _weight[var] = weight;
        model[var] = value;
    }

    virtual unsigned int nSoftVar() {
        unsigned int result = 0;
        for(auto w: _weight)
            if(w!=0)
                result++;
        return result;
    }

private:

    bool fullMinimize(VirtualSAT* solverForMinimize, std::set<int> &conflict, std::vector<int> &uselessLit, long timeRef) {
        MaLib::Chrono chrono;
        bool minimum = true;

        int B = 1000;
        //int B = 10000;

        if(timeRef > 60000000) {
            timeRef = 60000000;  // Hyperparameter
        }

        std::vector<int> removable;
        MonPrint("\t\t\t\t\tfullMinimize: Calculer Removable...");
        for(auto it = conflict.begin(); it != conflict.end(); ++it) {
            auto lit = *it;

            switch(solverForMinimize->solveLimited(conflict, B, lit)) {
            case 0:
                minimum = false;
                [[fallthrough]];
            case 1:
                break;

            case -1:
                removable.push_back(lit);
                break;

            default:
                assert(false);
            }
        }
        MonPrint("\t\t\t\t\tfullMinimize: removable = ", removable.size(), "/", conflict.size());

        if(removable.size() <= 1) {
            uselessLit = removable;
            for(auto lit: uselessLit) {
                conflict.erase(lit);
            }
            return minimum;
        }

        int maxLoop = 10000;
        if(removable.size() < 8) {
            maxLoop = 2*std::tgamma(removable.size());
        }

        chrono.tic();
        for(int i=0; i<maxLoop; i++) {
            std::set<int> wConflict = conflict;
            std::vector<int> tmp_uselessLit;
            for(auto lit: removable) {
                switch(solverForMinimize->solveLimited(wConflict, B, lit)) {
                    case 0:
                        minimum = false;
                        [[fallthrough]];
                    case 1:
                        break;

                    case -1:
                        wConflict.erase(lit);
                        tmp_uselessLit.push_back(lit);
                        break;

                    default:
                        assert(false);
                }
                /*
                if((timeRef*numberMinimizeThreadRunning <= chrono.tac()) && (B > 100)) {
                    MonPrint("\t\t\t\t\tfullMinimize: B=100");
                    B=100;
                }
                if((timeRef*numberMinimizeThreadRunning*2 <= chrono.tac()) && (B > 0)) {
                    MonPrint("\t\t\t\t\tfullMinimize: B=0");
                    B=0;
                }
                */
                /*
                if( (chrono.tacSec() >= _timeOutFastMinimize) && (chrono.tac() >= timeRef*(numberMinimizeThreadRunning+1)) && (B > 10) ) {
                    MonPrint("\t\t\t\t\tfullMinimize: B=10");
                    B=10;
                }
                */
            }

            if(tmp_uselessLit.size() > uselessLit.size()) {
                MonPrint("\t\t\t\t\tfullMinimize: newBest: ", tmp_uselessLit.size(), " removes.");
                uselessLit = tmp_uselessLit;
            }

            if(uselessLit.size() >= removable.size()-1) {
                MonPrint("\t\t\t\t\tfullMinimize: Optimal trouvé.");
                break;
            }

            if((i>=2) // Au moins 3 loops
                    && (timeRef*(1+numberMinimizeThreadRunning) <= chrono.tac())) {
                MonPrint("\t\t\t\t\tfullMinimize: TimeOut after ", (i+1), " loops");
                break;
            }

            std::random_shuffle(removable.begin(), removable.end());
        }

        for(auto lit: uselessLit) {
            conflict.erase(lit);
        }

        return minimum;
    }


    bool fastMinimize(VirtualSAT* solverForMinimize, std::list<int> &conflict) {
        int B = 1;

        Chrono chrono;
        for(auto it = conflict.begin(); it != conflict.end(); ++it) {

            if(chrono.tacSec() > _timeOutFastMinimize) {  // Hyperparameter
                MonPrint("TIMEOUT fastMinimize!");
                return false;
            }

            auto lit = *it;
            it = conflict.erase(it);

            switch(solverForMinimize->solveLimited(conflict, B)) {
            case 0:
                [[fallthrough]];
            case 1:
                it = conflict.insert(it, lit);
                break;

            case -1:
                break;

            default:
                assert(false);
            }
        }

        return true;
    }
};


EvalMaxSAT::~EvalMaxSAT() {
    CL_ConflictToMinimize.close();
    CL_LitToUnrelax.close();
    CL_LitToRelax.close();
    CL_CardToAdd.close();

    delete solver;
    for(auto s: solverForMinimize) {
        delete s;
    }
}



#endif // EVALMAXSAT_SLK178903R_H
