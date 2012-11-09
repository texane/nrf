#include <stdint.h>
#include <avr/io.h>
#include <stdint.h>


/* mode is selected by sending a configuration value

   shockurst mode:
   when sending, the processing is done by the nordic chip
   by fly so that the total time required to send a given
   amount of data is less, reducing the total power.

   direct mode:
   data sent by the mcu directly modulates the carrier, no
   support from the chip.

   configuration mode:
   3wire interface (cs, clk, data)

   standby mode:
   user for power reduction but short startup times. the
   configuration word is maintained. current consumption is
   on the order of 30uA.

   power down mode:
   current consumption less than 1uA. configuration word
   is maintained.
 */

/* framing: preamble addr payload crc */

/* configuration */

/* address width */
#define CONFIG_ADDR_SIZE 1
#define CONFIG_PAYLOAD_SIZE 8
#define CONFIG_LADDR 0x2a
#define CONFIG_RADDR 0x2b

/* io pins */

/* do not use portb2, spi cs unused by 3wire */

/* chip select, configuration mode */
#define CONFIG_CS_PIN 0
#define CONFIG_CS_PORT PORTB
#define CONFIG_CS_DDR DDRB
/* chip enable */
#define CONFIG_CE_PIN 1
#define CONFIG_PORT_CE PORTB
#define CONFIG_CE_DDR DDRB
/* clock */
#define CONFIG_CLK_PIN 5
#define CONFIG_CLK_PORT PORTB
#define CONFIG_CLK_DDR DDRB
/* data, 3 wire interface */
#define CONFIG_DAT_PIN 3
#define CONFIG_DAT_PORT PORTB
#define CONFIG_DAT_DDR DDRB
/* data ready */
#define CONFIG_DR_PIN 6
#define CONFIG_DR_PORT PINB
#define CONFIG_DR_DDR DDRB


/* 3 wires spi routines */

static inline void spi_setup_master(void)
{
  /* doc8161.pdf, ch.18 */

  /* cs, pb2 */
  DDRB |= (1 << 2);
  PORTB |= 1 << 2;

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

#if 1 /* FIXME: needed for sd_read_block to work */
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

static void spi_read(uint8_t* s, regtype_t len)
{
  for (; len; --len, ++s) *s = spi_read_uint8();
}

/* nrf24l01a routines */

typedef struct nrf24l01a_confblock_t
{
  uint8_t rx_en: 1;
  uint8_t rf_ch: 7;
  uint8_t rf_pwr: 2;
  uint8_t xo_f: 3;
  uint8_t rfdr_sb: 1;
  uint8_t cm: 1;
  uint8_t rx2_en: 1;
  uint8_t crc_en: 1;
  uint8_t crc_l: 1;
  uint8_t addr_w: 6;
  uint8_t addr1[40];
  uint8_t addr2[40];
  uint8_t data1_w: 8;
  uint8_t data2_w: 8;
  uint32_t test: 24;
} __attribute__((packed)) nrf24l01a_confblock_t;

static void nrf24l01a_wait_200us(void)
{
  /* 3200 cycles at 16mhz */
  unsigned int i;
  for (i = 0; i < 1000; ++i) __asm__ __volatile__ ("nop\n\t");
}

static void nrf24l01a_wait_3ms(void)
{
  unsigned int i;
  for (i = 0; i < 30; ++i) nrf24l01a_wait_200us();
}

static inline void nrf24l01a_set_conf(void)
{
  /* set configuration mode */

  cbi(CONFIG_CE_PORT, CONFIG_CE_PIN);
  /* in this order */
  sbi(CONFIG_CS_PORT, CONFIG_CS_PIN);
}

static void nrf24l01a_setup(void)
{
  static const nrf24l01a_confblock_t conf =
  {
    0, /* rx_en */
    0, /* f = 2400mhz + rf_ch * 1mhz */
    3, /* 0 dbm */
    3, /* on board 16mhz xtal */
    1, /* 1mbps baud rate */
    1, /* shock burst mode */
    0, /* disable channel 2 */
    1, /* enable onchip crc gen and check */
    0, /* 8 bits crc */
    CONFIG_ADDR_SIZE * 8, /* 8 bits addresses */
    { CONFIG_LADDR, 0, }, /* local address */
    { 0, },
    CONFIG_PAYLOAD_SIZE * 8, /* 64 bits channel 1 payload */
    0, /* no channel 2 payload */
    0 /* open tx, closed rx */
  };

  /* must be signed */
  int i;

  spi_setup_master();

  CONFIG_CE_DDR |= 1 << CONFIG_CE_PIN;
  CONFIG_CS_DDR |= 1 << CONFIG_CS_PIN;
  CONFIG_DAT_DDR |= 1 << CONFIG_DAT_PIN;
  CONFIG_CLK_DDR |= 1 << CONFIG_CLK_PIN;
  CONFIG_DR_DDR &= ~(0 << CONFIG_DR_PIN);

  /* wait for 3ms (pwr_dwn -> conf_mode) */
  nrf24l01a_wait_3ms();
  nrf24l01a_set_conf();

  /* configuration is loaded msb first */
  for (i = (sizeof(conf) - 1); i >= 0; --i)
    spi_write_uint8(((const uint8_t*)&conf)[i]);
}

static void nrf24l01a_set_standby(void)
{
  /* set standby mode */
  cbi(CONFIG_CE_PORT, CONFIG_CE_PIN);
  cbi(CONFIG_CS_PORT, CONFIG_CS_PIN);
}

static inline void nrf24l01a_set_active(void)
{
  /* set active mode */
  sbi(CONFIG_CE_PORT, CONFIG_CE_PIN);
}

static void nrf24l01a_set_tx(void)
{
  /* switch to active tx mode */

  /* wait for 200 us if leaving standby */
  nrf24l01a_wait_200us();
  nrf24l01a_set_conf();

  /* transmit mode */
  spi_write_uint8(0);

  /* leave conf to active mode */
  nrf24l01a_set_active();
}

static void nrf24l01a_set_rx(void)
{
  /* switch to active rx mode */

  /* wait for 200 us if leaving standby */
  nrf24l01a_wait_200us();
  nrf24l01a_set_conf();

  /* receive mode */
  spi_write_uint8(1);

  /* leave conf to active mode */
  nrf24l01a_set_active();
}

static void nrf24l01a_send(uint8_t addr, uint8_t* s)
{
  /* assume nrf24l01a_set_tx called */
  /* assume n <= CONFIG_PAYLOAD_SIZE */

  /* mcu sets ce high, to enable onboard data processing */
  /* addr and payload are clocked into the nrf24l01a */
  /* mcu sets ce low, activating the transmission */
  /* chip returns to standby when finished */

  unsigned int i;

  sbi(CONFIG_CE_PORT, CONFIG_CE_PIN);
  spi_write_uint8(addr);
  for (i = 0; i < CONFIG_PAYLOAD_SIZE; ++i, ++s) spi_write_uint8(*s);
  cbi(CONFIG_CE_PORT, CONFIG_CE_PIN);
}

static unsigned int nrf24l01a_recv(uint8_t* s)
{
  /* assume nrf24l01a_set_rx called */
  /* assume s is CONFIG_PAYLOAD_SIZE wide */

  /* set ce high */
  /* dr is set high when correct data is received */
  /* mcu clocks the data at a suitable rate */
  /* dr is set low when all the data are got */
  /* if ce has been kept low by the mcu, nrf ready to receive */

  unsigned int i;

  while ((CONFIG_DR_PORT & (1 << CONFIG_DR_PIN)) == 0) ;

  /* set ce low to disable data reception */
  cbi(CONFIG_PORT_CE, CONFIG_CE_PIN);

  for (i = 0; i < CONFIG_PAYLOAD_SIZE; ++i)
  {
    s[i] = spi_read_uint8();

    /* all the data have been got */
    if ((CONFIG_DR_PORT & (1 << CONFIG_DR_PIN) == 0)) break ;
  }

  /* read dummy data if any */
  while (CONFIG_DR_PORT & (1 << CONFIG_DR_PIN)) spi_read_uint8();

  return i;
}


/* main */

int main(int ac, char** av)
{
  nrf24l01a_setup();
  return 0;
}
