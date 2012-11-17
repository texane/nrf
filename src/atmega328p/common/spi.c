#ifndef SPI_C_INCLUDED
#define SPI_C_INCLUDED 1


#include <stdint.h>
#include <avr/io.h>


static inline void spi_setup_master(void)
{
  /* doc8161.pdf, ch.18 */

  /* ss is used by avr spi to determine master */
  /* set output mode even if pb2 not used by us */
  DDRB |= (1 << 2);

  /* spi output pins: sck pb5, mosi pb3 */
  DDRB |= (1 << 5) | (1 << 3);

  /* spi input pins: miso pb4 */
  DDRB &= ~(1 << 4);
  /* disable pullup (already by default) */
  PORTB &= ~(1 << 4);

  /* enable spi, msb first, master, freq / 128 (125khz), sck low idle */
  SPCR = (1 << SPE) | (1 << MSTR) | (3 << SPR0);

  /* clear double speed */
  SPSR &= ~(1 << SPI2X);
}

static inline uint8_t spi_set_cpol(void)
{
  const uint8_t spcr = SPCR;
  SPCR |= (1 << CPOL);
  return spcr;
}

static inline void spi_restore_cpol(uint8_t spcr)
{
  if (spcr & (1 << CPOL)) SPCR |= 1 << CPOL;
  else SPCR &= ~(1 << CPOL);
}

static inline void spi_set_sck_freq(uint8_t x)
{
  /* x one of SPI_SCK_FREQ_FOSCX */
  /* where spi sck = fosc / X */
  /* see atmega328 specs, table 18.5 */
#define SPI_SCK_FREQ_FOSC2 ((1 << 2) | 0)
#define SPI_SCK_FREQ_FOSC4 ((0 << 2) | 0)
#define SPI_SCK_FREQ_FOSC8 ((1 << 2) | 1)
#define SPI_SCK_FREQ_FOSC16 ((0 << 2) | 1)
#define SPI_SCK_FREQ_FOSC32 ((1 << 2) | 2)
#define SPI_SCK_FREQ_FOSC64 ((0 << 2) | 2)
#define SPI_SCK_FREQ_FOSC128 ((0 << 2) | 3)

  SPCR &= ~(3 << SPR0);
  SPCR |= (x & 3) << SPR0;

  SPSR &= ~(1 << SPI2X);
  SPSR |= (((x >> 2) & 1) << SPI2X);
}

static inline void spi_write_uint8(uint8_t x)
{
  /* write the byte and wait for transmission */

#if 0 /* FIXME: needed for sd_read_block to work */
  __asm__ __volatile__ ("nop\n\t");
  __asm__ __volatile__ ("nop\n\t");
#endif

  SPDR = x;

#if 0
  if (SPSR & (1 << WCOL))
  {
#if 1
    PRINT_FAIL();
#else
    /* access SPDR */
    volatile uint8_t fubar = SPDR;
    __asm__ __volatile__ ("" :"=m"(fubar));
    goto redo;
#endif
  }
#endif

  while ((SPSR & (1 << SPIF)) == 0) ;
}

static void spi_write_uint16(uint16_t x)
{
  spi_write_uint8((x >> 8) & 0xff);
  spi_write_uint8((x >> 0) & 0xff);
}

static void spi_write(const uint8_t* s, uint8_t len)
{
  for (; len; --len, ++s) spi_write_uint8(*s);
}

static inline uint8_t spi_read_uint8(void)
{
  /* by writing to mosi, 8 clock pulses are generated
     allowing the slave to transmit its register on miso
   */
  spi_write_uint8(0xff);
  return SPDR;
}

static void spi_read(uint8_t* s, uint8_t len)
{
  for (; len; --len, ++s) *s = spi_read_uint8();
}


#endif /* SPI_C_INCLUDED */
