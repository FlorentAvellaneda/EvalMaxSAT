

# EvalMaxSAT

Paper link: [A Short Description of the Solver EvalMaxSAT](http://florent.avellaneda.free.fr/dl/EvalMaxSAT.pdf)

## Introduction

EvalMaxSAT is a MaxSAT solver written in modern C++ language mainly using the Standard Template Library (STL).
The solver is built on top of the SAT solver CaDiCaL [1], but any other SAT solver can easily be used instead.
EvalMaxSAT is based on the OLL algorithm [2] originally implemented in the MSCG MaxSAT solver [3], [4] and then reused in the RC2 solver [5].
The Totalizer Encoding [6] is used to represent cardinalities and the implementation reuses the code from the PySAT’s ITotalizer [7].

[1]: A. Biere, K. Fazekas, M. Fleury and M. Heisinger, "CaDiCaL, Kissat, Paracooba, Plingeling and Treengeling Entering the SAT Competition 2020"

[2]: A. Morgado, C. Dodaro, and J. Marques-Silva, "Core-guided MaxSAT with soft cardinality constraints"

[3]: A. Morgado, A. Ignatiev, and J. Marques-Silva, "MSCG: robust core-guided maxsat solving"

[4]: A. Ignatiev, A. Morgado, V. M. Manquinho, I. Lynce, and J. Marques-Silva, "Progression in maximum satisfiability"

[5]: A. Ignatiev, A. Morgado, and J. Marques-Silva, "RC2: an efficient MaxSAT solver"

[6]: R. Martins, S. Joshi, V. M. Manquinho, and I. Lynce, "Reflections on incremental cardinality constraints for MaxSat"

[7]: A. Ignatiev, A. Morgado, and J. Marques-Silva, "Pysat: A python toolkit for prototyping with SAT oracles"

## Dependencies

Dependencies already included:
- CaDiCaL : http://fmv.jku.at/cadical/
- MCQD : http://insilab.org/mcqd-ml/
- CLI11 : https://github.com/CLIUtils/CLI11

## Installation

```bash
git clone https://github.com/FlorentAvellaneda/EvalMaxSAT.git
mkdir EvalMaxSAT/build
cd EvalMaxSAT/build
cmake ..
make
sudo make install
```

## Usage

```bash
$ EvalMaxSAT_bin --help
EvalMaxSAT Solver
Usage: ./EvalMaxSAT_bin [OPTIONS] file

Positionals:
  file TEXT:FILE REQUIRED     File with the formula to be solved (wcnf format)

Options:
  -h,--help                   Print this help message and exit
  --minRefTime UINT           Minimal reference time to improve unsat core (default = 1)
  --maxRefTime UINT           Maximal reference time to improve unsat core (default = 300)
  --TCT UINT                  Target Computation Time (default = 3600)
  --coefAVG FLOAT             Average coef on ref time (default = 1.66)
  --coefInit FLOAT            Initial coef on ref time (default = 10)
  --old                       Use old output format
  --bench UINT                Bench mode
  --noDS                      Unactivate Delay Strategy
  --noMS                      Unactivate Multisolve Strategy
  --noUBS                     Unactivate UB Strategy
```

Example:

```bash
$ ./EvalMaxSAT_bin log.8.wcsp.log.wcnf 
o 18446744073709551615
o 2
s OPTIMUM FOUND
v 00100101010011111
c Total time : 0.00165765s
```

## Use as a library

```c++
#include "EvalMaxSAT.h"

int main(int argc, char *argv[]) {
    EvalMaxSAT solver;

    // Create 3 variables
    int a = solver.newVar();
    int b = solver.newVar();
    int c = solver.newVar();

    // Add hard clauses
    solver.addClause({-a, -b});        // !a or !b

    // Add soft clauses
    solver.addClause({a, b}, 1);       // a or b
    solver.addClause({c}, 1);          // c
    solver.addClause({a, -c}, 1);      // a or !c
    solver.addClause({b, -c}, 1);      // b or !c

    ////// PRINT SOLUTION //////////////////
    if(!solver.solve()) {
        std::cout << "s UNSATISFIABLE" << std::endl;
        return 0;
    }
    std::cout << "s OPTIMUM FOUND" << std::endl;
    std::cout << "o " << solver.getCost() << std::endl;
    std::cout << "a = " << solver.getValue(a) << std::endl;
    std::cout << "b = " << solver.getValue(b) << std::endl;
    std::cout << "c = " << solver.getValue(c) << std::endl;
    ///////////////////////////////////////
}
```

You can reuse and adapt the CMakeLists.txt file to compile.

