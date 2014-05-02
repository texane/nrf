#if defined (CONFIG_LM4F120XL)

#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"

static unsigned long softspi_sck_addr;
static unsigned long softspi_miso_addr;
static unsigned long softspi_mosi_addr;
static unsigned long softspi_csn_addr;

static inline void softspi_setup_master(void)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

  softspi_sck_addr = GPIO_PORTD_BASE + (GPIO_PIN_0 << 2);
  softspi_miso_addr = GPIO_PORTD_BASE + (GPIO_PIN_1 << 2);
  softspi_mosi_addr = GPIO_PORTD_BASE + (GPIO_PIN_2 << 2);
  softspi_csn_addr = GPIO_PORTD_BASE + (GPIO_PIN_3 << 2);

  MAP_GPIOPinTypeGPIOOutput
  (
   softspi_sck_addr & 0xfffff000,
   (softspi_sck_addr & 0x00000fff) >> 2
  );

  MAP_GPIOPinTypeGPIOInput
  (
   softspi_miso_addr & 0xfffff000,
   (softspi_miso_addr & 0x00000fff) >> 2
  );

  MAP_GPIOPinTypeGPIOOutput
  (
   softspi_mosi_addr & 0xfffff000,
   (softspi_mosi_addr & 0x00000fff) >> 2
  );

  MAP_GPIOPinTypeGPIOOutput
  (
   softspi_csn_addr & 0xfffff000,
   (softspi_csn_addr & 0x00000fff) >> 2
  );
}

static inline void softspi_clk_low(void)
{
  HWREG(softspi_sck_addr) = 0x00;
}

static inline void softspi_clk_high(void)
{
  HWREG(softspi_sck_addr) = 0xff;
}

static inline void softspi_mosi_low(void)
{
  HWREG(softspi_mosi_addr) = 0x00;
}

static inline void softspi_mosi_high(void)
{
  HWREG(softspi_mosi_addr) = 0xff;
}

static inline void softspi_cs_low(void)
{
  HWREG(softspi_csn_addr) = 0x00;
}

static inline void softspi_cs_high(void)
{
  HWREG(softspi_csn_addr) = 0xff;
}

#if (SOFTSPI_DONT_USE_MISO == 0)
static inline uint8_t softspi_is_miso(void)
{
  return HWREG(softspi_miso_addr);
}
#endif

#else /* ATMEGA328P */

#include <stdint.h>
#include <avr/io.h>

#ifndef SOFTSPI_DONT_USE_MISO
#define SOFTSPI_DONT_USE_MISO 0
#endif

static void softspi_setup_master(void)
{
  SOFTSPI_CLK_DDR |= SOFTSPI_CLK_MASK;
  SOFTSPI_MOSI_DDR |= SOFTSPI_MOSI_MASK;

#if (SOFTSPI_DONT_USE_MISO == 0)
  SOFTSPI_MISO_DDR &= ~SOFTSPI_MISO_MASK;
#endif
}

static inline void softspi_clk_low(void)
{
  SOFTSPI_CLK_PORT &= ~SOFTSPI_CLK_MASK;
}

static inline void softspi_clk_high(void)
{
  SOFTSPI_CLK_PORT |= SOFTSPI_CLK_MASK;
}

static inline void softspi_mosi_low(void)
{
  SOFTSPI_MOSI_PORT &= ~SOFTSPI_MOSI_MASK;
}

static inline void softspi_mosi_high(void)
{
  SOFTSPI_MOSI_PORT |= SOFTSPI_MOSI_MASK;
}

#if (SOFTSPI_DONT_USE_MISO == 0)
static inline uint8_t softspi_is_miso(void)
{
  return SOFTSPI_MISO_PIN & SOFTSPI_MISO_MASK;
}
#endif

#endif /* ATMEGA328P */

static inline void softspi_set_sck_freq(uint8_t x)
{
}

static inline void softspi_write_bit(uint8_t x, uint8_t m)
{
  /* 5 insns per bit */

#if defined(SOFTSPI_MODE_0) /* cpol == 0, cpha == 0 */
  if (x & m) softspi_mosi_high(); else softspi_mosi_low();
#endif

  softspi_clk_high();

#if !defined(SOFTSPI_MODE_0)
  if (x & m) softspi_mosi_high(); else softspi_mosi_low();
#endif

  softspi_clk_low();
}

static void softspi_write_uint8(uint8_t x)
{
  /* transmit msb first, sample at clock falling edge */

  softspi_write_bit(x, (1 << 7));
  softspi_write_bit(x, (1 << 6));
  softspi_write_bit(x, (1 << 5));
  softspi_write_bit(x, (1 << 4));
  softspi_write_bit(x, (1 << 3));
  softspi_write_bit(x, (1 << 2));
  softspi_write_bit(x, (1 << 1));
  softspi_write_bit(x, (1 << 0));
}

static inline void softspi_write_uint16(uint16_t x)
{
  softspi_write_uint8((uint8_t)(x >> 8));
  softspi_write_uint8((uint8_t)(x & 0xff));
}

static void softspi_write(const uint8_t* s, uint8_t n)
{
  for (; n != 0; --n, ++s) softspi_write_uint8(*s);
}

#if (SOFTSPI_DONT_USE_MISO == 0)

static inline void softspi_read_bit(uint8_t* x, uint8_t i)
{
  softspi_clk_high();

#if defined(SOFTSPI_MODE_0) /* cpol == 0, cpha == 0 */
  if (softspi_is_miso()) *x |= 1 << i;
#endif

  softspi_clk_low();

#if !defined(SOFTSPI_MODE_0)
  if (softspi_is_miso()) *x |= 1 << i;
#endif
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
  /* msB ordering */
  const uint8_t x = softspi_read_uint8();
  return ((uint16_t)x << 8) | (uint16_t)softspi_read_uint8();
}

static void softspi_read(uint8_t* s, uint8_t n)
{
  for (; n != 0; --n, ++s) *s = softspi_read_uint8();
}

#endif /* SOFTSPI_DONT_USE_MISO == 0 */
