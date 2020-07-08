#ifndef MALIB__MATH_H
#define MALIB__MATH_H

#include <vector>

namespace MaLib {

    unsigned int pow2(unsigned int p) {
        return 1 << p;
    }

    unsigned int bin2int(const std::vector<bool> &bin) {
        unsigned int result = 0;

        for(unsigned int i=0; i<bin.size(); i++) {
            if(bin[i]) {
                result += pow2(i);
            }
        }

        return result;
    }

}

#endif // MALIB__MATH_H
