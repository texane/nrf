#include <stdint.h>
#include <avr/io.h>

#define DAC7554_SOFTSPI 1

#if (DAC7554_SOFTSPI == 0)

#include "../../common/spi.c"
#define DAC7554_SYNC_MASK (1 << 2)

#else

#define DAC7554_SYNC_MASK (1 << 2)
#define DAC7554_SOFTSPI_CLK_DDR DDRD
#define DAC7554_SOFTSPI_CLK_PORT PORTD
#define DAC7554_SOFTSPI_CLK_MASK (1 << 3)
#define DAC7554_SOFTSPI_MOSI_DDR DDRD
#define DAC7554_SOFTSPI_MOSI_PORT PORTD
#define DAC7554_SOFTSPI_MOSI_MASK (1 << 4)

static void softspi_setup_master(void)
{
  /* mosi only */

  DAC7554_SOFTSPI_CLK_DDR |= DAC7554_SOFTSPI_CLK_MASK;
  DAC7554_SOFTSPI_MOSI_DDR |= DAC7554_SOFTSPI_MOSI_MASK;
}

static inline void softspi_clk_low(void)
{
  DAC7554_SOFTSPI_CLK_PORT &= ~DAC7554_SOFTSPI_CLK_MASK;
}

static inline void softspi_clk_high(void)
{
  DAC7554_SOFTSPI_CLK_PORT |= DAC7554_SOFTSPI_CLK_MASK;
}

static inline void softspi_mosi_low(void)
{
  DAC7554_SOFTSPI_MOSI_PORT &= ~DAC7554_SOFTSPI_MOSI_MASK;
}

static inline void softspi_mosi_high(void)
{
  DAC7554_SOFTSPI_MOSI_PORT |= DAC7554_SOFTSPI_MOSI_MASK;
}

static inline void softspi_write_bit(uint8_t x, uint8_t m)
{
  /* dac7554 samples at clock falling edge */

  /* 5 insns per bit */

  softspi_clk_high();
  if (x & m) softspi_mosi_high(); else softspi_mosi_low();
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

#endif /* DAC7554_SOFTSPI */


/* reference: SLAS399A.pdf */

static inline void dac7554_sync_low(void)
{
  PORTD &= ~DAC7554_SYNC_MASK;
}

static inline void dac7554_sync_high(void)
{
  PORTD |= DAC7554_SYNC_MASK;
}

static void dac7554_setup(void)
{
  DDRD |= DAC7554_SYNC_MASK;
  dac7554_sync_high();

#if (DAC7554_SOFTSPI == 1)
  softspi_setup_master();
#endif
}

static inline void dac7554_write(uint16_t data, uint16_t chan)
{
  /* assume spi initialized */

  /* refer to SLAS399A, p.14 table.1 for commands */

  /* assume: 0 <= data < 4096 */
  /* assume: 0 <= chan <= 3 */

  /* 2 to update both input and DAC registers */  
  const uint16_t cmd = (2 << 14) | (chan << 12) | data;

#if (DAC7554_SOFTSPI == 0)
  const uint8_t cpol = spi_set_cpol();
#endif

  /* pulse sync */
  dac7554_sync_high();
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");
  dac7554_sync_low();

  /* wait for SCLK falling edge setup time, 4ns */
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");

#if (DAC7554_SOFTSPI == 0)
  spi_write_uint16(cmd);
#else
  softspi_write_uint16(cmd);
#endif

  /* wait for SLCK to rise (0 ns) and set high */
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");
  dac7554_sync_high();

#if (DAC7554_SOFTSPI == 0)
  spi_restore_cpol(cpol);
#endif
}
