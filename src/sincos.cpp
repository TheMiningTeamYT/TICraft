#include <cmath>
#include "sincos.hpp"
#include "fixedpoint.h"

Fixed24 fastSinCos::table[sineTableSize];

void fastSinCos::generateTable() {
    for (unsigned int i = 0; i < sineTableSize; i++) {
        fastSinCos::table[i] = sinf(static_cast<float>(i)*sineTableGap);
    }
}

Fixed24 fastSinCos::sin(Fixed24 angle) {
    // Make sure value is in right range first
    if (angle < static_cast<Fixed24>(0)) {
        angle += static_cast<Fixed24>(static_cast<float>(2*M_PI));
    }
    if (angle >= static_cast<Fixed24>(static_cast<float>(2*M_PI))) {
        angle -= static_cast<Fixed24>(static_cast<float>(2*M_PI));
    }
    return fastSinCos::table[static_cast<int>(angle*static_cast<Fixed24>(sineTableGapReciprocal))];
}

Fixed24 fastSinCos::cos(Fixed24 angle) {
    return fastSinCos::sin(angle + static_cast<Fixed24>(static_cast<float>(M_PI_2)));
}