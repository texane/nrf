#include <stdint.h>
#include "../common/softspi.c"

/* TODO: digital interface page 29 */
/* TODO: add to softspi */
/* TODO: extract softspi */
/* TODO: parametrize softspi at compile time */

static inline void softspi_read_bit(uint8_t* x, uint8_t i)
{
  /* read at falling edge */

  softspi_clk_high();
#if 0
  /* no need, atmega328p clock below 50mhz */
  /* softspi_wait_clk(); */
#endif
  softspi_clk_low();

  if (softspi_miso_is_high()) *x |= 1 << i;
}

static uint8_t softspi_read_uint8(void)
{
  /* receive msb first, sample at clock falling edge */

  /* must be initialized to 0 */
  uint8_t x = 0;

  softspi_read_bit(&x, 7);
  softspi_read_bit(&x, 6);
  softspi_read_bit(&x, 5);
  softspi_read_bit(&x, 4);
  softspi_read_bit(&x, 3);
  softspi_read_bit(&x, 2);
  softspi_read_bit(&x, 1);
  softspi_read_bit(&x, 0);

  return x;
}

static inline uint16_t softspi_read_uint16(void)
{
  /* lsB first */
  const uint8_t x = softspi_read_uint8();
  return ((uint16_t)x << 8) | (uint16_t)softspi_read_uint8();
}

static void adc8327_setup(void)
{
  DDRD |= ADC8327_SYNC_MASK;
  adc8327_sync_high();

  softspi_setup_master();
}

static uint16_t adc8327_read(void)
{
  /* falling edge */
}
