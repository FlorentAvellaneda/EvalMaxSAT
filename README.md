

# EvalMaxSAT

Paper link: [A Short Description of the Solver EvalMaxSAT](http://florent.avellaneda.free.fr/dl/EvalMaxSAT.pdf)

## Introduction

EvalMaxSAT is a MaxSAT solver written in modern C++ language mainly using the Standard Template Library (STL).
The solver is built on top of the SAT solver Glucose [1], but any other SAT solver can easily be used instead.
EvalMaxSAT is based on the OLL algorithm [2] originally implemented in the MSCG MaxSAT solver [3], [4] and then reused in the RC2 solver [5].
The Totalizer Encoding [6] is used to represent cardinalities and the implementation reuses the code from the PySAT’s ITotalizer [7].

[1]: G. Audemard, J. Lagniez, and L. Simon, "Improving glucose for incremental SAT solving with assumptions: Application to MUS extraction"

[2]: A. Morgado, C. Dodaro, and J. Marques-Silva, "Core-guided MaxSAT with soft cardinality constraints"

[3]: A. Morgado, A. Ignatiev, and J. Marques-Silva, "MSCG: robust core-guided maxsat solving"

[4]: A. Ignatiev, A. Morgado, V. M. Manquinho, I. Lynce, and J. Marques-Silva, "Progression in maximum satisfiability"

[5]: A. Ignatiev, A. Morgado, and J. Marques-Silva, "RC2: an efficient MaxSAT solver"

[6]: R. Martins, S. Joshi, V. M. Manquinho, and I. Lynce, "Reflections on incremental cardinality constraints for MaxSat"

[7]: A. Ignatiev, A. Morgado, and J. Marques-Silva, "Pysat: A python toolkit for prototyping with SAT oracles"

## Dependencies

Dependencies already included:
- glucose : https://www.labri.fr/perso/lsimon/glucose/
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
EvalMaxSAT V0.6
Usage: EvalMaxSAT\_bin [OPTIONS] WCNF\_file

Positionals:
  WCNF_file TEXT:FILE REQUIRED
                              wcnf file

Options:
  -h,--help                   Print this help message and exit
  -p UINT                     Number of minimization threads (default = 0)
  --timout_fast UINT          Timeout in second for fast minimize (default = 60)
  --coef_minimize UINT        Multiplying coefficient of the time spent to minimize cores (default = 2)
```

Example:

```bash
$ EvalMaxSAT_bin wqueens14_12.wcsp.dir.wcnf
s OPTIMUM FOUND
o 4
v -1 -2 -3 -4 -5 -6 7 -8 -9 -10 -11 -12 -13 -14 -15 -16 -17 -18 -19 -20 -21 -22 -23 -24 25 -26 -27 -28 -29 -30 -31 -32 -33 -34 -35 36 -37 -38 -39 -40 -41 -42 -43 -44 45 -46 -47 -48 -49 -50 -51 -52 -53 -54 -55 -56 -57 -58 -59 -60 -61 -62 -63 -64 65 -66 -67 -68 -69 -70 -71 -72 -73 -74 -75 -76 -77 -78 -79 -80 -81 -82 83 -84 -85 -86 -87 -88 89 -90 -91 -92 -93 -94 -95 -96 -97 -98 99 -100 -101 -102 -103 -104 -105 -106 -107 -108 -109 -110 -111 -112 -113 -114 -115 -116 -117 118 -119 -120 -121 -122 -123 -124 -125 -126 -127 -128 -129 -130 -131 -132 -133 -134 -135 136 -137 -138 -139 -140 -141 -142 -143 -144 -145 -146 -147 -148 -149 -150 -151 152 -153 -154 -155 -156 -157 -158 -159 -160 -161 -162 -163 -164 -165 -166 -167 168 -169 -170 -171 172 -173 -174 -175 -176 -177 -178 -179 -180 -181 -182 -183 184 -185 -186 -187 -188 -189 -190 -191 -192 -193 -194 -195 -196
c Total time: 795.657 ms

```

## Use as a library

```c++
#include "EvalMaxSAT.h"

int main(int argc, char *argv[]) {
    unsigned int paralleleThread = 0;
    auto solver = new EvalMaxSAT(paralleleThread);

    // Create 3 variables
    int a = solver->newVar();
    int b = solver->newVar();
    int c = solver->newVar();

    // Add hard clauses
    solver->addClause({-a, -b});                // !a or !b
    auto card = solver->newCard({a, b, c});
    solver->addClause( {*card <= 2} );          // a + b + c <= 2

    // Add soft clauses
    solver->addWeightedClause({a, b}, 1);       // a or b
    solver->addWeightedClause({c}, 1);          // c
    solver->addWeightedClause({a, -c}, 1);      // a or !c
    solver->addWeightedClause({b, -c}, 1);      // b or !c

    ////// PRINT SOLUTION //////////////////
    if(!solver->solve()) {
        std::cout << "s UNSATISFIABLE" << std::endl;
        return 0;
    }
    std::cout << "s OPTIMUM FOUND" << std::endl;
    std::cout << "o " << solver->getCost() << std::endl;
    std::cout << "a = " << solver->getValue(a) << std::endl;
    std::cout << "b = " << solver->getValue(b) << std::endl;
    std::cout << "c = " << solver->getValue(c) << std::endl;
    ///////////////////////////////////////
}
```

You can reuse and adapt the CMakeLists.txt file to compile.

## Benchmark
The single thread version of EvalMaxSAT was ranked second in the [MaxSAT Evaluation 2020](https://maxsat-evaluations.github.io/2020/).

![img](http://florent.avellaneda.free.fr/maxsat2020.jpg)



