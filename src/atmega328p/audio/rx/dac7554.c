#include <stdint.h>
#include <avr/io.h>
#include "../../common/spi.c"

/* reference: SLAS399A.pdf */

static inline void dac7554_spi_cs_low(void)
{
#define DAC7554_SPI_CS_MASK (1 << 2)
  PORTD &= ~DAC7554_SPI_CS_MASK;
}

static inline void dac7554_spi_cs_high(void)
{
  PORTD |= DAC7554_SPI_CS_MASK;
}

static void dac7554_setup(void)
{
  /* assume spi initialized */
#if 0
  /* spi_setup_master() */
  /* spi_setup_master(SPI_SCK_FREQ_FOSC2); */
#endif

  DDRD |= DAC7554_SPI_CS_MASK;
  dac7554_spi_cs_high();
}

static inline void dac7554_write(uint16_t data, uint16_t chan)
{
  /* refer to SLAS399A, p.14 table.1 for commands */

  /* assume: 0 <= data < 4096 */
  /* assume: 0 <= chan <= 3 */

  /* 2 to update both input and DAC registers */  
  const uint16_t cmd = (2 << 14) | (chan << 12) | data;

  dac7554_spi_cs_low();

  /* wait for SCLK falling edge setup time, 4ns */
  __asm__ __volatile__ ("nop");

  spi_write_uint16(cmd);

  /* wait for SLCK to rise (0 ns) and set high */
  __asm__ __volatile__ ("nop");
  dac7554_spi_cs_high();
}
