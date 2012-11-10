#include <stdint.h>
#include <avr/io.h>
#include "../../common/spi.c"


/* reference: SLAS399A.pdf */

static inline void dac7554_sync_low(void)
{
#define DAC7554_SYNC_MASK (1 << 0)
  PORTB &= ~DAC7554_SYNC_MASK;
}

static inline void dac7554_sync_high(void)
{
  PORTB |= DAC7554_SYNC_MASK;
}

static void dac7554_setup(void)
{
  /* sync pin controled with PB0, 8 */
  DDRB |= DAC7554_SYNC_MASK;
  dac7554_sync_high();
}

static inline void dac7554_write(uint16_t data, uint16_t chan)
{
  /* assume spi initialized */

  /* refer to SLAS399A, p.14 table.1 for commands */

  /* assume: 0 <= data < 4096 */
  /* assume: 0 <= chan <= 3 */

  /* 2 to update both input and DAC registers */  
  const uint16_t cmd = (2 << 14) | (chan << 12) | data;

  const uint8_t cpol = spi_set_cpol();

  /* pulse sync */
  dac7554_sync_high();
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");
  dac7554_sync_low();

  /* wait for SCLK falling edge setup time, 4ns */
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");

  spi_write_uint16(cmd);

  /* wait for SLCK to rise (0 ns) and set high */
  __asm__ __volatile__ ("nop");
  __asm__ __volatile__ ("nop");
  dac7554_sync_high();

  spi_restore_cpol(cpol);
}
