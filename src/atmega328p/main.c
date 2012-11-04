/* nrf24l01p library */

#include <stdint.h>
#include <avr/io.h>
#include <stdint.h>


/* only for debuggging. */

#define NRF24L01P_UART 1

#if (NRF24L01P_UART == 1) /* uart */

static inline void set_baud_rate(long baud)
{
  uint16_t UBRR0_value = ((F_CPU / 16 + baud / 2) / baud - 1);
  UBRR0H = UBRR0_value >> 8;
  UBRR0L = UBRR0_value;
}

static void uart_setup(void)
{
  /* #define CONFIG_FOSC (F_CPU * 2) */
  /* const uint16_t x = CONFIG_FOSC / (16 * BAUDS) - 1; */
#if 0 /* (bauds == 9600) */
  const uint16_t x = 206;
#elif 0 /* (bauds == 115200) */
  const uint16_t x = 16;
#elif 0 /* (bauds == 500000) */
  const uint16_t x = 3;
#elif 0 /* (bauds == 1000000) */
  const uint16_t x = 1;
#endif

  set_baud_rate(9600);

  /* baud doubler off  - Only needed on Uno XXX */
  UCSR0A &= ~(1 << U2X0);

  UCSR0B = 1 << TXEN0;

  /* default to 8n1 framing */
  UCSR0C = (3 << 1);
}

static void uart_write(uint8_t* s, uint8_t n)
{
  for (; n; --n, ++s)
  {
    /* wait for transmit buffer to be empty */
    while (!(UCSR0A & (1 << UDRE0))) ;
    UDR0 = *s;
  }

  /* wait for last byte to be sent */
  while ((UCSR0A & (1 << 6)) == 0) ;
}

static inline uint8_t nibble(uint32_t x, uint8_t i)
{
  return (x >> (i * 4)) & 0xf;
}

static inline uint8_t hex(uint8_t x)
{
  return (x >= 0xa) ? 'a' + x - 0xa : '0' + x;
}

static uint8_t* uint32_to_string(uint32_t x)
{
  static uint8_t buf[8];

  buf[7] = hex(nibble(x, 0));
  buf[6] = hex(nibble(x, 1));
  buf[5] = hex(nibble(x, 2));
  buf[4] = hex(nibble(x, 3));
  buf[3] = hex(nibble(x, 4));
  buf[2] = hex(nibble(x, 5));
  buf[1] = hex(nibble(x, 6));
  buf[0] = hex(nibble(x, 7));

  return buf;
}

#endif /* NRF24L01P_UART */


/* set to 0 if spi implemented elsewhere. */

#define NRF24L01P_SPI 1

#if (NRF24L01P_SPI == 1)

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

static void spi_read(uint8_t* s, uint8_t len)
{
  for (; len; --len, ++s) *s = spi_read_uint8();
}

static inline void spi_cs_low(void)
{
  PORTB &= ~(1 << 2);
}

static inline void spi_cs_high(void)
{
  PORTB |= 1 << 2;
}

#endif /* NRF24L01P_SPI */


/* nrf24l01p */

/* pinout */
#define NRF24L01P_IO_CE_MASK (1 << 0)
#define NRF24L01P_IO_CE_DDR DDRB
#define NRF24L01P_IO_CE_PORT PORTB
#define NRF24L01P_IO_IRQ_MASK (1 << 1)
#define NRF24L01P_IO_IRQ_DDR DDRB
#define NRF24L01P_IO_IRQ_PIN PINB

/* payload size, in bytes */
#define NRF24L01P_PAYLOAD_SIZE 8
/* 3 bytes wide local addresses */
static const uint8_t NRF24L01P_LADDR[3] = { 0x2a, 0x2a, 0x2a };

static void wait_130us(void)
{
  volatile uint16_t x;
  for (x = 0; x < 2000; ++x) __asm__ __volatile__ ("nop\n\t");
}

static void wait_5ms(void)
{
  volatile uint8_t x;
  for (x = 0; x < 40; ++x) wait_130us();
}

static void wait_100ms(void)
{
  volatile uint8_t x;
  for (x = 0; x < 20; ++x) wait_5ms();
}

/* [ operationnal modes ]
   powerdown: configuration registers and spi maintained
   standby: same as powerdown but short startup times
   tx or rx: all the corresponding modules activated

   [ enhanced shockburst ]
   packet processing is handled by the chip, reducing
   power consumption. it includes:
   . incoming packet checks and disassembly
   . outgoing packet assembly and retransmission
   packets are acknowledged or retransmitted after a
   programmable delay and retries. data can be included
   in the ack packet.

   [ shockburst compatibility ]
   enhanced shockburst must be disabled for backward
   compatibility with shockburst packet framing and
   absence of retransmission features.

   [ packet format ]
   preamble: 1 byte, for transceiver synchronization
   address: 3 upto 5 bytes
   control: 9 bits for payload length, pid and no_ack
   payload: upto 32 bytes
   crc: 1 or 2 bytes

   [ control interface ]
   spi at upto 10mbps. msbit lsbyte first.

   [ interrupt ]
   active low irq pin.
 */


/* control interface */

/* command context. keep global to reduce the generated code size */
static uint8_t nrf24l01p_cmd_op;
static uint8_t nrf24l01p_cmd_buf[8]; /* max(NRF24L01P_PAYLOAD_SIZE, 5) */
static uint8_t nrf24l01p_cmd_len;

/* commands */
#define NRF24L01P_CMD_R_REG 0x00
#define NRF24L01P_CMD_W_REG 0x20
#define NRF24L01P_CMD_R_RX_PAYLOAD 0x31
#define NRF24L01P_CMD_W_TX_PAYLOAD 0xa0
#define NRF24L01P_CMD_FLUSH_TX 0xe1
#define NRF24L01P_CMD_FLUSH_RX 0xe2
#define NRF24L01P_CMD_W_TX_PAYLOAD_NOACK 0xb0
#define NRF24L01P_CMD_NOP 0xff

/* registers */
#define NRF24L01P_REG_CONFIG 0x00
#define NRF24L01P_REG_EN_AA 0x01
#define NRF24L01P_REG_EN_RXADDR 0x02
#define NRF24L01P_REG_SETUP_AW 0x03
#define NRF24L01P_REG_SETUP_RETR 0x04
#define NRF24L01P_REG_RF_CH 0x05
#define NRF24L01P_REG_RF_SETUP 0x06
#define NRF24L01P_REG_STATUS 0x07
#define NRF24L01P_REG_OBSERVE_TX 0x08
#define NRF24L01P_REG_RPD 0x09
#define NRF24L01P_REG_RX_ADDR_P0 0x0a
#define NRF24L01P_REG_RX_ADDR_P1 0x0b
#define NRF24L01P_REG_RX_ADDR_P2 0x0c
#define NRF24L01P_REG_RX_ADDR_P3 0x0d
#define NRF24L01P_REG_RX_ADDR_P4 0x0e
#define NRF24L01P_REG_RX_ADDR_P5 0x0f
#define NRF24L01P_REG_TX_ADDR 0x10
#define NRF24L01P_REG_RX_PW_P0 0x11
#define NRF24L01P_REG_RX_PW_P1 0x12
#define NRF24L01P_REG_RX_PW_P2 0x13
#define NRF24L01P_REG_RX_PW_P3 0x14
#define NRF24L01P_REG_RX_PW_P4 0x15
#define NRF24L01P_REG_RX_PW_P5 0x16
#define NRF24L01P_REG_FIFO_STATUS 0x17
#define NRF24L01P_REG_DYNPD 0x1c
#define NRF24L01P_REG_FEATURE 0x1d

static void nrf24l01p_cmd_prolog(void)
{
  spi_cs_high();
  spi_cs_low();
  spi_write_uint8(nrf24l01p_cmd_op);
}

static void nrf24l01p_cmd_write(void)
{
  /* assume op, buf, len filled */
  nrf24l01p_cmd_prolog();
  spi_write(nrf24l01p_cmd_buf, nrf24l01p_cmd_len);
  spi_cs_low();
}

static void nrf24l01p_cmd_read(void)
{
  nrf24l01p_cmd_prolog();
  spi_read(nrf24l01p_cmd_buf, nrf24l01p_cmd_len);
  spi_cs_low();
}

static inline void nrf24l01p_cmd_make(uint8_t op, uint8_t len)
{
  nrf24l01p_cmd_op = op;
  nrf24l01p_cmd_len = len;
}

static uint8_t nrf24l01p_read_reg8(uint8_t r)
{
  /* read a 8 bit register */
  nrf24l01p_cmd_make(NRF24L01P_CMD_R_REG | r, 1);
  nrf24l01p_cmd_read();
  return nrf24l01p_cmd_buf[0];
}

static void nrf24l01p_write_reg8(uint8_t r, uint8_t x)
{
  nrf24l01p_cmd_make(NRF24L01P_CMD_W_REG | r, 1);
  nrf24l01p_cmd_buf[0] = x;
  nrf24l01p_cmd_write();
}

static void nrf24l01p_or_reg8(uint8_t r, uint8_t m)
{
  /* m the mask */
  const uint8_t x = nrf24l01p_read_reg8(r);
  nrf24l01p_write_reg8(r, x | m);
}

static void nrf24l01p_and_reg8(uint8_t r, uint8_t m)
{
  /* m the mask */
  const uint8_t x = nrf24l01p_read_reg8(r);
  nrf24l01p_write_reg8(r, x & m);
}

static void nrf24l01p_and_or_reg8(uint8_t r, uint8_t am, uint8_t om)
{
  /* am the anded mask */
  /* om the ored mask */
  const uint8_t x = nrf24l01p_read_reg8(r);
  nrf24l01p_write_reg8(r, (x & am) | om);
}

static inline void nrf24l01p_read_reg40(uint8_t r)
{
  nrf24l01p_cmd_make(NRF24L01P_CMD_R_REG | r, 5);
  nrf24l01p_cmd_read();
  /* result is in nrf24l01p_cmd_buf */
}

static inline void nrf24l01p_write_reg40(uint8_t r)
{
  /* assume nrf24l01p_cmd_buf filled */
  nrf24l01p_cmd_make(NRF24L01P_CMD_W_REG | r, 5);
  nrf24l01p_cmd_write();
}

static uint8_t nrf24l01p_read_status(void)
{
  /* status is always clocked out on miso after a cs hilo trans */
  nrf24l01p_cmd_make(NRF24L01P_CMD_NOP, 1);
  nrf24l01p_cmd_read();
  return nrf24l01p_cmd_buf[0];
}


/* operating modes */

static inline void nrf24l01p_set_powerdown(void)
{
  nrf24l01p_and_reg8(NRF24L01P_REG_CONFIG, ~(1 << 1));
}

static inline void nrf24l01p_set_standby(void)
{
  /* assume tx or rx mode */
  NRF24L01P_IO_CE_PORT &= ~NRF24L01P_IO_CE_MASK;
}

static inline void nrf24l01p_set_tx(void)
{
  /* set PRIM_RX low */
  nrf24l01p_and_reg8(NRF24L01P_REG_CONFIG, ~(1 << 0));
}

static inline void nrf24l01p_set_rx(void)
{
  /* set PRIM_RX high */
  nrf24l01p_or_reg8(NRF24L01P_REG_CONFIG, 1 << 1);
}

static void nrf24l01p_powerdown_to_standby(void)
{
  nrf24l01p_set_standby();
  wait_5ms();
}

static void nrf24l01p_standby_to_rx(void)
{
  nrf24l01p_set_rx();
  NRF24L01P_IO_CE_PORT |= NRF24L01P_IO_CE_MASK;
  wait_130us();
}

static void nrf24l01p_standby_to_tx(void)
{
  nrf24l01p_set_tx();
  NRF24L01P_IO_CE_PORT |= NRF24L01P_IO_CE_MASK;
  wait_130us();
}

static void nrf24l01p_rx_to_tx(void)
{
  nrf24l01p_set_tx();
  wait_130us();
}

static void nrf24l01p_tx_to_rx(void)
{
  nrf24l01p_set_rx();
  wait_130us();
}

static void nrf24l01p_setup(void)
{
  /* setup spi */
#if (NRF24L01P_SPI == 1)
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);
#endif /* NRF24L01P_SPI */

  /* pinouts */
  NRF24L01P_IO_CE_DDR |= NRF24L01P_IO_CE_MASK;
  NRF24L01P_IO_IRQ_DDR &= ~NRF24L01P_IO_IRQ_MASK;

  /* wait for 100ms on reset */
  wait_100ms();

  /* configure registers */

  /* powerdown after reset */
  nrf24l01p_write_reg8(NRF24L01P_REG_CONFIG, ~(1 << 1));

  /* disable autoack */
  nrf24l01p_write_reg8(NRF24L01P_REG_EN_AA, 0);

  /* disable crc */
  nrf24l01p_and_reg8(NRF24L01P_REG_CONFIG, ~(1 << 3));

  /* enable only pipe 0 */
  nrf24l01p_write_reg8(NRF24L01P_REG_EN_RXADDR, 1 << 0);

  /* 3 bytes wide addr */
#define NRF24L01P_ADDR_WIDTH_3 1
#define NRF24L01P_ADDR_WIDTH_4 2
#define NRF24L01P_ADDR_WIDTH_5 3
  nrf24l01p_write_reg8(NRF24L01P_REG_SETUP_AW, NRF24L01P_ADDR_WIDTH_3);

  /* auto retransmit disabled */
  nrf24l01p_write_reg8(NRF24L01P_REG_SETUP_RETR, 0);

  /* 2400 mhz channel */
  nrf24l01p_write_reg8(NRF24L01P_REG_RF_CH, 0);

  /* 2mpbs, 0dbm gain */
  nrf24l01p_write_reg8(NRF24L01P_REG_RF_SETUP, 0x0e);

  /* pipe0 receive address */
  nrf24l01p_cmd_buf[0] = NRF24L01P_LADDR[0];
  nrf24l01p_cmd_buf[1] = NRF24L01P_LADDR[1];
  nrf24l01p_cmd_buf[2] = NRF24L01P_LADDR[2];
  nrf24l01p_write_reg40(NRF24L01P_REG_RX_ADDR_P0);

  /* transmit address */
  nrf24l01p_cmd_buf[0] = 0;
  nrf24l01p_cmd_buf[1] = 0;
  nrf24l01p_cmd_buf[2] = 0;
  nrf24l01p_write_reg40(NRF24L01P_REG_TX_ADDR);

  /* pipe0 payload size */
  nrf24l01p_write_reg8(NRF24L01P_REG_RX_PW_P0, NRF24L01P_PAYLOAD_SIZE);

  /* pipe0 disable dynamic payload */
  nrf24l01p_write_reg8(NRF24L01P_REG_DYNPD, 0);

  /* disable dynamic payload, payload with ack, noack */
  nrf24l01p_write_reg8(NRF24L01P_REG_FEATURE, 0);
}

static inline void nrf24l01p_enable_crc8(void)
{
  nrf24l01p_and_or_reg8(NRF24L01P_REG_CONFIG, ~(3 << 2), 1 << 3);
}

static inline void nrf24l01p_enable_crc16(void)
{
  nrf24l01p_or_reg8(NRF24L01P_REG_CONFIG, 3 << 2);
}

static inline void nrf24l01p_disable_crc(void)
{
  nrf24l01p_and_reg8(NRF24L01P_REG_CONFIG, ~(1 << 3));
}

static inline void nrf24l01p_set_addr_width(uint8_t x)
{
  nrf24l01p_and_or_reg8(NRF24L01P_REG_SETUP_AW, ~(3 << 0), x);
}

static inline void nrf24l01p_set_rate(uint8_t x)
{
  /* note: this is a mask */
#define NRF24L01P_RATE_1MBPS (0 << 3)
#define NRF24L01P_RATE_2MBPS (1 << 3)
#define NRF24L01P_RATE_250KBPS (4 << 3)
#define NRF24L01P_RATE_MASK (5 << 3)
  nrf24l01p_and_or_reg8(NRF24L01P_REG_RF_SETUP, ~NRF24L01P_RATE_MASK, x);
}

static inline void nrf24l01p_set_chan(uint8_t x)
{
  nrf24l01p_write_reg8(NRF24L01P_REG_RF_CH, x);
}

static void nrf24l01p_set_raddr(const uint8_t* addr)
{
  /* set the remote address */
  /* assume a 3 bytes wide addr */
  nrf24l01p_cmd_buf[0] = addr[0];
  nrf24l01p_cmd_buf[1] = addr[1];
  nrf24l01p_cmd_buf[2] = addr[2];
  nrf24l01p_write_reg40(NRF24L01P_REG_TX_ADDR);
}

static void nrf24l01p_enable_art(uint8_t n, uint8_t d)
{
  /* enable automatic retransmission */

  /* n the retransmission count */
  /* d the retransmission delay */
}

static void nrf24l01p_disable_art(void)
{
  /* disable automatic retransmission */
}

static void nrf24l01p_send(void)
{
  /* send one payload, of size NRF24L01P_PAYLOAD_SIZE */

  /* a the addr */
  /* s the buffer to send */

  /* TODO: nrf24l01p_cmd_write(NRF24L01P_CMD_W_TX_PAYLOAD); */
}

static void nrf24l01p_send_noack(uint8_t a, const uint8_t* s)
{
  /* in no ack mode, the chip directly returns to standy */
}

static void nrf24l01p_recv(void)
{
  /* receive one payload */
  /* payload available in nrf24l01p_cmd_buf */
  nrf24l01p_cmd_make(NRF24L01P_CMD_W_TX_PAYLOAD, NRF24L01P_PAYLOAD_SIZE);
  nrf24l01p_cmd_read();
}

static inline uint8_t nrf24l01p_read_irq(void)
{
#define NRF24L01P_IRQ_MASK_RX_DR (1 << 6)
#define NRF24L01P_IRQ_MASK_TX_DS (1 << 5)
#define NRF24L01P_IRQ_MASK_MAX_RT (1 << 4)
#define NRF24L01P_IRQ_MASK_ALL (7 << 4)

  /* irq signal is active low */
  if ((NRF24L01P_IO_IRQ_PIN & NRF24L01P_IO_IRQ_MASK) == 0)
  {
    const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_CONFIG);
    /* reset irq by writing back 1s to the source bits */
    nrf24l01p_write_reg8(NRF24L01P_REG_CONFIG, x);
    return x;
  }

  return 0;
}

static inline unsigned int nrf24l01p_is_rx(void)
{
  /* return non zero if rx related interrupt */
  return nrf24l01p_read_irq() & NRF24L01P_IRQ_MASK_RX_DR;
}


/* main */

int main(void)
{
#if (NRF24L01P_UART == 1)
  uart_setup();
  uart_write((uint8_t*)"o\r\n", 3);
#endif /* NRF24L01P_UART */

  nrf24l01p_setup();

  /* sparkfun usb serial board configuration */
  nrf24l01p_enable_crc16();
  /* auto ack disabled */
  /* auto retransmit disabled */
  /* 5 bytes addr width */
  nrf24l01p_set_addr_width(NRF24L01P_ADDR_WIDTH_5);
  /* 1mbps, 0dbm */
  nrf24l01p_set_rate(NRF24L01P_RATE_1MBPS);
  /* channel 2 */
  nrf24l01p_set_chan(2);
  /* rx address */
  nrf24l01p_cmd_buf[0] = 0xe7;
  nrf24l01p_cmd_buf[1] = 0xe7;
  nrf24l01p_cmd_buf[2] = 0xe7;
  nrf24l01p_cmd_buf[3] = 0xe7;
  nrf24l01p_cmd_buf[4] = 0xe7;
  nrf24l01p_write_reg40(NRF24L01P_REG_RX_ADDR_P0);

  nrf24l01p_powerdown_to_standby();
  nrf24l01p_standby_to_rx();

 redo_receive:
  while (nrf24l01p_is_rx() == 0) ;
  uart_write((uint8_t*)"x\r\n", 3);
  nrf24l01p_recv();
  goto redo_receive;

  return 0;
}
