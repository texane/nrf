#ifndef SPI_LM4F120XL_C_INCLUDED
#define SPI_LM4F120XL_C_INCLUDED

#define CONFIG_LM4F120XL 1
#define SOFTSPI_DONT_USE_MISO 0

#include "softspi.c"

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
