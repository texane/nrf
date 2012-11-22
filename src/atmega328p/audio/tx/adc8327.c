#include <stdint.h>
#include "../../common/softspi.c"


#define ADC8327_CS_MASK (1 << 2)
#define ADC8327_CS_DDR DDRD
#define ADC8327_CS_PORT PORTD


/* TODO: digital interface page 29 */
/* TODO: add to softspi */
/* TODO: extract softspi */
/* TODO: parametrize softspi at compile time */

static inline void adc8327_cs_low(void)
{
  ADC8327_CS_PORT &= ~(ADC8327_CS_MASK);
}

static inline void adc8327_cs_high(void)
{
  ADC8327_CS_PORT |= ADC8327_CS_MASK;
}

static inline void adc8327_write_cmd(uint16_t cmd)
{
  softspi_write_uint16(cmd);
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

static inline void adc8327_cs_pulse(void)
{
  adc8327_cs_high();
  /* no need for wait cycle on atmega328p */
  adc8327_cs_low();
}

static uint16_t adc8327_read_common(uint8_t op)
{
  uint16_t x;

  adc8327_cs_pulse();

  adc8327_write_cmd(adc8327_make_cmd0(op));
  x = softspi_read_uint16();

  adc8327_cs_high();

  return x;
}

static inline uint16_t adc8327_read_data(void)
{
  return adc8327_read_common(ADC8327_OP_READ_DATA);
}

static uint16_t adc8327_read_cfr(void)
{
  return adc8327_read_common(ADC8327_OP_READ_CFR);
}

static void adc8327_write_cfr(uint16_t x)
{
  adc8327_cs_pulse();
  adc8327_write_cmd(adc8327_make_cmd16(ADC8327_OP_WRITE_CFR, x));
  adc8327_cs_high();
}

static inline void adc8327_setup(void)
{
  ADC8327_CS_DDR |= ADC8327_CS_MASK;
  adc8327_cs_high();

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
}

static inline void adc8327_start_free_running(void)
{
  /* default is: */
  /* auto channel selected enabled */
  /* internal conversion clock */
  /* manual conversion trigger */

  uint16_t x;

  x = adc8327_read_cfr();

  /* automatic conversion triggering */
  x &= ~ADC8327_CFR_TRIG;

  adc8327_write_cfr(x);
}

static inline void adc8327_stop(void)
{
  /* TODO */
}

static inline uint16_t adc8327_read(void)
{
  return adc8327_read_data();
}
