/* generate a 8 bits sin wave sample array */

#include <stdio.h>
#include <math.h>
#include <stdint.h>

static inline uint8_t q(double x)
{
  return (uint8_t)ceil((256.0 * (x + 1.0)) / 2.0);
}

int main(int ac, char** av)
{
  static const double ftone = FTONE;
  static const double fsampl = FSAMPL;
  static const unsigned int nsampl = 256;

  unsigned int i;

  for (i = 0; i < nsampl; ++i)
  {
    const double t = (double)i / fsampl;
    const uint8_t x = q(sin(2.0 * M_PI * ftone * t));
    if (i && ((i % 8) == 0)) printf("\n");
    printf("0x%02x", x);
    if (i != (nsampl - 1)) printf(", ");
  }
  printf("\n");

  return 0;
}
