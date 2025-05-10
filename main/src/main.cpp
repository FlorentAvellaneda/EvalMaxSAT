#include <iostream>
#include <cassert>
#include <csignal>
#include <chrono>

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <memory>

#include "EvalMaxSAT.h"
#include "EvalMaxSAT.h"
#include "CLI11.hpp"

using namespace MaLib;

Chrono TotalChrono("c Total time");

std::string cur_file;
std::unique_ptr<EvalMaxSAT<Solver_cadical>> monMaxSat;
bool oldOutputFormat = false;
unsigned int bench = 0;

void signalHandler( int signum ) {
    std::cout << "c Interrupt signal (" << signum << ") received."<< std::endl;

    if(monMaxSat != nullptr) {
        auto solution = monMaxSat->getSolution();
        if(bench) {
                std::cout << cur_file << "\t" << calculateCost(cur_file, solution) << "\t" << TotalChrono.tacSec() << std::endl;
        } else {
            t_weight cost = calculateCost(cur_file, solution);

            if(cost != -1) {
                if(oldOutputFormat) {
                    std::cout << "o " << calculateCost(cur_file, solution) << std::endl;
                    std::cout << "s SATISFIABLE" << std::endl;
                    std::cout << "v";
                    for(unsigned int i=1; i<solution.size(); i++) {
                        if(solution[i]) {
                            std::cout << " " << i;
                        } else {
                            std::cout << " -" << i;
                        }
                    }
                    std::cout << std::endl;
                } else {
                    std::cout << "o " << calculateCost(cur_file, solution) << std::endl;
                    std::cout << "s SATISFIABLE" << std::endl;
                    //std::cout << "o " << calculateCost(file, solution) << std::endl;
                    std::cout << "v ";
                    for(unsigned int i=1; i<solution.size(); i++) {
                        std::cout << solution[i];
                    }
                    std::cout << std::endl;
                }
                exit(10); // The solver finds a solution satisfying the hard clauses but does not prove it to be optimal
            } else {
                std::cout << "s UNKNOWN" << std::endl;
                exit(0); // The solver cannot find a solution satisfying the hard clauses or prove unsatisfiability
            }
        }
    }


    exit(0); // Bench mode
}


template<class SOLVER>
std::tuple<bool, std::vector<bool>, t_weight> solveFile(SOLVER *monMaxSat, std::string file, double targetComputationTime=3600) {
    cur_file = file;

    if(!parse(file, monMaxSat)) {
        std::cerr << "Unable to read the file " << file << std::endl;
        assert(false);
        return std::make_tuple<bool, std::vector<bool>, t_weight>(false, {},-1);
    }

    monMaxSat->setTargetComputationTime( targetComputationTime - TotalChrono.tacSec() );
    if(monMaxSat->isWeighted() == false) {
        monMaxSat->disableOptimize();
    }
    if(!monMaxSat->solve()) {
        //std::cout << "s UNSATISFIABLE" << std::endl;
        return std::make_tuple<bool, std::vector<bool>, t_weight>(false, {},-1);
    }
    auto solution = monMaxSat->getSolution();
    std::cout << "c nombre de var = " << solution.size() << std::endl;
    assert(monMaxSat->getCost() == calculateCost(file, solution));

    return {true, solution, calculateCost(file, solution)};
}




void test() {

    bool pas_de_new_soft = true;

    std::vector<int> var2var;
    var2var.push_back(0); // fake

    std::vector<bool> is_soft_var;
    EvalMaxSAT<Solver_cadical> S;
    if(pas_de_new_soft == false)
        S.setIncremental(true);
    std::vector< std::vector<int> > hard_clauses;

    std::vector< std::vector<int> > soft_clauses;

    is_soft_var.push_back(false); // fake
    for(int i=1; i<=50; i++) {
        if(MaLib::MonRand::getBool()) {
            is_soft_var.push_back(true);
            int v = S.newSoftVar(true, 1);
            var2var.push_back(v);
            assert(v == i);
        } else {
            is_soft_var.push_back(false);
            int v = S.newVar(true);
            var2var.push_back(v);
            assert(v == i);
        }
    }

    for(unsigned int i=0;;i++) {
        if(MaLib::MonRand::get(1, 10) == 1) {
            if(!pas_de_new_soft && MaLib::MonRand::getBool()) {
                is_soft_var.push_back(true);
                int v = S.newSoftVar(true, 1);
                var2var.push_back(v);
            } else {
                is_soft_var.push_back(false);
                int v = S.newVar(true);
                var2var.push_back(v);
            }
        }

        std::vector<int> clause;
        std::vector<int> clause2;

        int size = MaLib::MonRand::get(1, 5);
        for(unsigned int i=0; i<size; i++) {
            clause.push_back(  MaLib::MonRand::get(1, is_soft_var.size()-1) ); 
            assert(abs(clause.back()) <= is_soft_var.size()-1);
            clause2.push_back( var2var[clause.back()] );
            if(MaLib::MonRand::getBool()) {
                clause.back() = -clause.back();
                clause2.back() = -clause2.back();
            }
        }
        //std::cout << "c add " << clause << std::endl;

        if(pas_de_new_soft || MaLib::MonRand::getBool()) {
            S.addClause(clause2);
            hard_clauses.push_back(clause);
        } else {
            S.addClause(clause2, 1);
            soft_clauses.push_back(clause);
        }

        //std::cout << "create S2" << std::endl;
        EvalMaxSAT<Solver_cadical> S2;
        for(int i=1; i<=is_soft_var.size()-1; i++) {
            if(is_soft_var[i]) {
                int v = S2.newSoftVar(true, 1);
                assert(v == i);
            } else {
                int v = S2.newVar(true);
                assert(v == i);
            }
        }
        for(auto &clause: hard_clauses) {
            S2.addClause(clause);
        }
        for(auto &clause: soft_clauses) {
            S2.addClause(clause, 1);
        }

        //std::cout << "solve S" << std::endl;
        auto res = S.solve();
        //std::cout << "solve S2" << std::endl;
        auto res2 = S2.solve();

        if(res != res2) {
            std::cout << "c " << res << " != " << res2 << std::endl;
            std::cout << "c " << S.getCost() << " != " << S2.getCost() << std::endl;
            assert(false);
            exit(-1);
        }

        if(res == false) {
            break;
        }

        if(S.getCost() != S2.getCost()) {
            std::cout << "c " << S.getCost() << " != " << S2.getCost() << std::endl;
            assert(false);
            exit(-1);
        }
    }



}

int main(int argc, char *argv[])
{
    assert([](){std::cout << "c Assertion activated. For better performance, compile the project with assertions disabled. (-DNDEBUG)" << std::endl; return true;}());

    for(unsigned int nb=6; ; nb++) {
        MaLib::MonRand::seed(nb);
        srand(nb);
        std::cout << "c Test " << nb << std::endl;
        test();
    }
    return 0;


    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    ///////////////////////////
    /// PARSE ARG
    ///
        CLI::App app{"EvalMaxSAT Solver"};

        std::string file;
        app.add_option("file", file, "File with the formula to be solved (wcnf format)")->check(CLI::ExistingFile)->required();

        unsigned int minimalRefTime=1;
        app.add_option("--minRefTime", minimalRefTime, toString("Minimal reference time to improve unsat core (default = ",minimalRefTime,")"));

        unsigned int maximalRefTime=5*60;
        app.add_option("--maxRefTime", maximalRefTime, toString("Maximal reference time to improve unsat core (default = ",maximalRefTime,")"));

        unsigned int targetComputationTime = 60*60;
        app.add_option("--TCT", targetComputationTime, toString("Target Computation Time (default = ",targetComputationTime,")"));

        double coefOnRefTime = 1.66;
        app.add_option("--coefAVG", coefOnRefTime, toString("Average coef on ref time (default = ",coefOnRefTime,")"));

        double initialCoef = 10;
        app.add_option("--coefInit", initialCoef, toString("Initial coef on ref time (default = ",initialCoef,")"));

        app.add_flag("--old", oldOutputFormat, "Use old output format");

        app.add_option("--bench", bench, "Bench mode");

        bool noDS=false;
        app.add_flag("--noDS", noDS, "Unactivate Delay Strategy");

        bool noMS=false;
        app.add_flag("--noMS", noMS, "Unactivate Multisolve Strategy");

        bool noUBS=false;
        app.add_flag("--noUBS", noUBS, "Unactivate UB Strategy");


        CLI11_PARSE(app, argc, argv);
    ///
    ////////////////////////////////////////

    monMaxSat = std::make_unique<EvalMaxSAT<Solver_cadical>>();
    monMaxSat->setCoef(initialCoef, coefOnRefTime);
    monMaxSat->setBoundRefTime(minimalRefTime, maximalRefTime);

    if(noDS) {
        monMaxSat->unactivateDelayStrategy();
    }
    if(noMS) {
        monMaxSat->unactivateMultiSolveStrategy();
    }
    if(noUBS) {
        monMaxSat->unactivateUBStrategy();
    }

    auto [sat, solution, cost] = solveFile(monMaxSat.get(), file, targetComputationTime);

    if(bench) {
        std::cout << file << "\t" << calculateCost(file, solution) << "\t" << TotalChrono.tacSec() << std::endl;
    } else {
        if(sat) {
            if(oldOutputFormat) {
                ////// PRINT SOLUTION OLD FORMAT //////////////////
                ///
                    std::cout << "o " << calculateCost(file, solution) << std::endl;
                    std::cout << "s OPTIMUM FOUND" << std::endl;
                    std::cout << "v";
                    for(unsigned int i=1; i<solution.size(); i++) {
                        if(solution[i]) {
                            std::cout << " " << i;
                        } else {
                            std::cout << " -" << i;
                        }
                    }
                    std::cout << std::endl;
                ///
                ///////////////////////////////////////
            } else {
                ////// PRINT SOLUTION NEW FORMAT //////////////////
                ///
                    std::cout << "o " << calculateCost(file, solution) << std::endl;
                    std::cout << "s OPTIMUM FOUND" << std::endl;
                    std::cout << "v ";
                    for(unsigned int i=1; i<solution.size(); i++) {
                        std::cout << solution[i];
                    }
                    std::cout << std::endl;
                ///
                ///////////////////////////////////////
            }
            return 30; // OPT
        } else {
            std::cout << "s UNSATISFIABLE" << std::endl;
            return 20; // UNSAT
        }
    }

    assert(calculateCost(file, solution) == cost);

    return 0; // Bench mode
}





