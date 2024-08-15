#pragma once
#include <cmath>
#include "fixedpoint.h"

// Ratios for converting from degrees to radians and vice versa
#define degRadRatio static_cast<float>(M_PI/180)
#define radDegRatio static_cast<float>(180/M_PI)

// How many entries the sine table has
#define sineTableSize 288
// How far apart each value in the sine table is
#define sineTableGap static_cast<float>((2*M_PI)/(sineTableSize))
// The reciprocal of the sine table gap (useful number)
#define sineTableGapReciprocal static_cast<float>(1/sineTableGap)

namespace fastSinCos {
    // The sine table (also used for cosine)
    extern Fixed24 table[sineTableSize];
    void generateTable();
    Fixed24 sin(Fixed24 angle);
    Fixed24 cos(Fixed24 angle);
}