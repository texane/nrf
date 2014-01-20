#ifndef SPI_LM4F120XL_C_INCLUDED
#define SPI_LM4F120XL_C_INCLUDED

#include <stdint.h>

#ifndef CONFIG_LM4F120XL
#define CONFIG_LM4F120XL 1
#endif

#define SOFTSPI_DONT_USE_MISO 0

#include "softspi.c"

#define SPI_SCK_FREQ_FOSC2 ((1 << 2) | 0)
#define SPI_SCK_FREQ_FOSC4 ((0 << 2) | 0)
#define SPI_SCK_FREQ_FOSC8 ((1 << 2) | 1)
#define SPI_SCK_FREQ_FOSC16 ((0 << 2) | 1)
#define SPI_SCK_FREQ_FOSC32 ((1 << 2) | 2)
#define SPI_SCK_FREQ_FOSC64 ((0 << 2) | 2)
#define SPI_SCK_FREQ_FOSC128 ((0 << 2) | 3)

static inline void spi_setup_master(void)
{
  softspi_setup_master();
}

static inline void spi_cs_low(void)
{
  softspi_cs_low();
}

static inline void spi_cs_high(void)
{
  softspi_cs_high();
}

static inline void spi_set_sck_freq(uint8_t x)
{
}

static inline void spi_write_uint8(uint8_t x)
{
  softspi_write_uint8(x);
}

static void spi_write(const uint8_t* s, uint8_t len)
{
  for (; len; --len, ++s) spi_write_uint8(*s);
}

static inline uint8_t spi_read_uint8(void)
{
  return softspi_read_uint8();
}

static void spi_read(uint8_t* s, uint8_t len)
{
  for (; len; --len, ++s) *s = spi_read_uint8();
}

#endif /* SPI_LM4F120XL_C_INCLUDED */
