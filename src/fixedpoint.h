#pragma once

/* Operators for 24-bit (12.12) fixed point arithmetic on the ez80.
 * 
 * Fixed point is preferred for this application because arithmetic is generally
 * faster, and the limited dynamic range is not necessary for the scene being
 * rendered.
 */

#define POINT 12

#include <tice.h>
#include "asmmath.h"
#include <math.h>
#include <stdlib.h>

extern const char* digits;

struct Fixed24 {
  int24_t n;

  Fixed24() {
    n = 0;
  }

  // Convert this integer to a fixed point representation
  Fixed24(int24_t _n) {
    n = _n << POINT;
  }

  // Approximates a given float as a fixed point number
  Fixed24(float _n) {
    n = (int24_t)(_n * (1 << POINT));
  }

  int24_t floor() {
    return fp_to_int_floor(n);
  }

  Fixed24 abs() {
    Fixed24 out;
    out.n = ::abs(n);
    return out;
  }

  int24_t round() {
    int24_t fraction = n % (1 << POINT);
    int24_t whole = n - fraction;
    if (fraction > (1 << (POINT - 1))) {
      whole += (1 << (POINT - 1));
    }
    return whole/(1 << POINT);
  }

  /* Rounds down the provided Fixed24 while preserving the last requested number
   * of digits. This is equivalent to
   *
   * floor(n * (2 ^ digits))
   * 
   * This is generally useful for sampling discrete values between 0 and 1
   * so long as the discrete space is 2 ^ digits in length
   */
  int24_t floor(uint8_t digits) {
    return n >> (POINT - digits);
  }

  Fixed24 operator+(Fixed24 v) const {
    Fixed24 out;

    out.n = n + v.n;

    return out;
  }

  Fixed24 operator-(Fixed24 v) const {
    Fixed24 out;

    out.n = n - v.n;

    return out;
  }

  Fixed24 operator*(Fixed24 v) const {
    Fixed24 out;

    // Invokes a specialized asm routine implemented in asmtest.asm
    out.n = fp_mul(n, v.n);

    return out;
  }

  Fixed24 operator-() const {
    Fixed24 out;
    
    out.n = -n;

    return out;
  }

  Fixed24 operator/(Fixed24 v) const {
    Fixed24 out;
    out.n = fp_div(n, v.n);
    return out;
  }

  void operator+=(Fixed24 v) {
    n += v.n;
  }

  void operator-=(Fixed24 v) {
    n -= v.n;
  }

  bool operator<(Fixed24 x) const {
    return n < x.n;
  }

  bool operator<=(Fixed24 x) const {
    return n <= x.n;
  }

  bool operator>(Fixed24 x) const {
    return n > x.n;
  }

  bool operator>=(Fixed24 x) const {
    return n >= x.n;
  }

  bool operator!=(Fixed24 x) const {
    return n != x.n;
  }
  bool operator==(Fixed24 x) const {
    return n == x.n;
  }
  operator int() const {
    return fp_to_int(n);
  }
  operator float() const {
    return (float)n/(float)(1 << POINT);
  }
};

/* Prints the hex digits of a fixed point number for debug usage
 */
void print_fixed(Fixed24 &x);

/* Computes the square root of a fixed point number and returns
 * the result as a fixed point number
 */
Fixed24 sqrt(Fixed24 &x);

/* Computes the square of a Fixed24 number.
 *
 * This uses a specialized multiplication implementation and is preferable
 * over multiplying the number with itself
 */
Fixed24 sqr(Fixed24 x);

/* Computes division of Fixed24 numbers.
 */
Fixed24 div(Fixed24 a, Fixed24 b);

/* Clamps the Fixed24 to be within the range 0 and 1 inclusively
 */
Fixed24 clamp01(Fixed24 x);

/* Clamps the Fixed24 to be no less than zero
 */
Fixed24 clamp0(Fixed24 x);

// A table of values of arcsin(x) / (pi / 2) used as a LUT in the asin function
extern int24_t asin_table[65];

/* Computes the arcsine of x, and returns a value in the range -1 to 1
 * (the actual arcsine divided by pi/2 since this value is more useful to us)
 */
Fixed24 asin(Fixed24 x);

/* Computes arctangent of the angle created between the x axis
 * and the line from the origin to (x, y)
 *
 * The result is also divided by pi/2 for simplicity in texture
 * sampling
 */
Fixed24 atan2(Fixed24 &x, Fixed24 &y);

Fixed24 abs(Fixed24 &x);
