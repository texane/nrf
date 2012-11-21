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


static inline void adc8327_write_cmd(uint16_t cmd)
{
}

static inline uint16_t adc8327_make_cmd0(uint8_t op)
{
#define ADC8327_OP_SEL_0 0x00
#define ADC8327_OP_SEL_1 0x01
#define ADC8327_OP_READ_CFR 0x0c
#define ADC8327_OP_READ_DATA 0x0d
#define ADC8327_OP_WRITE_CFR 0x0e
#define ADC8327_OP_WRITE_MODE 0x0f
  return op << 12;
}

static inline uint16_t adc8327_make_cmd4(uint8_t op, uint8_t x)
{
  return adc8327_make_cmd0(op) | (uint16_t)x;
}

static inline uint16_t adc8327_make_cmd16(uint8_t op, uint16_t x)
{
  return adc8327_make_cmd0(op) | x;
}

static uint16_t adc8327_read_data(void)
{
  adc8327_write_cmd(adc8327_make_cmd0(ADC8327_OP_READ_DATA));
  return adc8327_read16();
}

static uint16_t adc8327_read_cfr(void)
{
  adc8327_write_cmd(adc8327_make_cmd0(ADC8327_OP_READ_CFR));
  return adc8327_read16();
}

static void adc8327_write_cfr(uint16_t x)
{
  adc8327_write_cmd(adc8327_make_cmd16(ADC8327_OP_WRITE_CFR, x));
}

static void adc8327_setup(void)
{
  DDRD |= ADC8327_SYNC_MASK;
  adc8327_sync_high();

  softspi_setup_master();

  /* configuration register map, page 31 */
#define ADC8327_CFR_CHAN (1 << 11)
#define ADC8327_CFR_CONVCLK (1 << 10)
#define ADC8327_CFR_TRIG (1 << 9)
#define ADC8327_CFR_DONTCARE (1 << 8)
#define ADC8327_CFR_PIN10_POL (1 << 7)
#define ADC8327_CFR_PIN10_FUNC (1 << 6)
#define ADC8327_CFR_PIN10_IO (1 << 5)
#define ADC8327_CFR_NAP_AUTO (1 << 4)
#define ADC8327_CFR_NAP_PWRDN (1 << 3)
#define ADC8327_CFR_DEEP_PWRDN (1 << 2)
#define ADC8327_CFR_TAG (1 << 1)
#define ADC8327_CFR_RESET (1 << 0)

  /* TODO: free running mode */
}

static uint16_t adc8327_read(void)
{
  /* falling edge */
}
