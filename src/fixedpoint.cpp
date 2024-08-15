#include "fixedpoint.h"
#include <stdlib.h>

/* Clamps the Fixed24 to be no less than zero
 */
Fixed24 clamp0(Fixed24 x) {
  if (x < Fixed24(0)) return Fixed24(0);
  return x;
}

/* Clamps the Fixed24 to be within the range 0 and 1 inclusively
 */
Fixed24 clamp01(Fixed24 x) {
  if (x < Fixed24(0)) return Fixed24(0);
  if (x > Fixed24(1)) return Fixed24(1);
  return x;
}

/* Computes division of Fixed24 numbers.
 */
Fixed24 div(Fixed24 a, Fixed24 b) {

  Fixed24 out;
  out.n = fp_div(a.n, b.n);

  return out;
}

/* Computes the square of a Fixed24 number.
 *
 * This uses a specialized multiplication implementation and is preferable
 * over multiplying the number with itself
 */
Fixed24 sqr(Fixed24 x) {

  Fixed24 out;
  out.n = fp_sqr(x.n);

  return out;
}

/* Computes the square root of a fixed point number and returns
 * the result as a fixed point number
 */
Fixed24 sqrt(Fixed24 &x) {

  Fixed24 out;
  out.n = fp_sqrt(x.n);

  return out;
}

/* Prints the hex digits of a fixed point number for debug usage
 */
void print_fixed(Fixed24 &x) {
  char str[8];
  str[7] = '\0';

  uint24_t n = x.n;
  for (int8_t i = 6; i >= 0; i--) {
    // Skip the fourth character, which holds the decimal point
    if (i == 3) {
      str[i] = '.';
      continue;
    }

    // Extract the least significant digit and shift down the next
    str[i] = digits[n & 0xF];
    n = n >> 4;
  }

  os_PutStrFull(str);
  os_NewLine();
}

Fixed24 abs(Fixed24 &x) {
  Fixed24 out;
  out.n = ::abs(x.n);
  return out;
}

Fixed24 fromRaw(int _n) {
  Fixed24 out;
  out.n = _n;
  return out;
}