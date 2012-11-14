#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "../../common/spi.c"
#include "../../common/nrf24l01p.c"
#include "../../common/uart.c"
#include "dac7554.c"


/* fifo */

/* must be multiple of payload width, so no overflow */
/* must be pow2, so that modulo is an and operation */
#define FIFO_BUF_SIZE 256

#if (FIFO_BUF_SIZE <= 256)
typedef uint8_t fifo_index_t;
#else
typedef uint16_t fifo_index_t;
#endif

typedef struct fifo
{
  /* read write pointers */
  volatile fifo_index_t r;
  volatile fifo_index_t w;

  uint8_t buf[FIFO_BUF_SIZE];

} fifo_t;

static fifo_t fifo;

static inline fifo_index_t fifo_mod(fifo_index_t x)
{
#if (FIFO_BUF_SIZE == 256)
  /* wrapping automatically handled by 8 bits arithmetic */
  return x;
#else
  return x & ((fifo_index_t)sizeof(fifo.buf) - (fifo_index_t)1);
#endif
}

static inline void fifo_setup(void)
{
  fifo.w = 0;
  fifo.r = 0;
}

__attribute__((unused)) static void fifo_write_payload(void)
{
  /* write payload from nrf24l01p_cmd_buf */

  uint8_t i;

  for (i = 0; i < NRF24L01P_PAYLOAD_WIDTH; ++i)
  {
    fifo.buf[fifo.w] = nrf24l01p_cmd_buf[i];

    /* note that it is concurrent with reader interrupt */
    /* incrementing fifo.w comes after the actual write */
    /* so that the reader always access a valid location */
    fifo.w = fifo_mod(fifo.w + 1);
  }
}

static inline uint8_t fifo_is_empty(void)
{
  return fifo.w == fifo.r;
}

static inline fifo_index_t fifo_size(void)
{
  /* assume called by writer (main), concurrent with read (isr) */
  const fifo_index_t fifo_r  = fifo.r;
  if (fifo_r < fifo.w) return fifo.w - fifo_r;
  return sizeof(fifo.buf) - fifo_r + fifo.w;
}

static inline uint8_t fifo_read_uint8(void)
{
  /* index wrapping automatically handled */
  const uint8_t x = fifo.buf[fifo.r];
  fifo.r = fifo_mod(fifo.r + 1);
  return x;
}


/* timer1a compare on match handler */

#if (DAC7554_SOFTSPI == 0)
static volatile uint8_t lock_spi = 0;
#endif

ISR(TIMER1_COMPA_vect)
{
  uint8_t x;

  /* note that this is concurrent with fifo_write */
  /* the reader may miss a location in progress, but */
  /* never accesses an invalid one */
  if (fifo_is_empty()) return ;

  x = fifo_read_uint8();

  /* 12 bits dac, extend for now */
#if (DAC7554_SOFTSPI == 0)
  if (lock_spi == 0)
#endif
    dac7554_write((uint16_t)x << 4, 0);
}

static inline void on_nrf24l01p_irq(void)
{
#if (DAC7554_SOFTSPI == 0)
  lock_spi = 1;
#endif

  /* no overflow, fifo.w mutiple of payload width */
  nrf24l01p_read_rx_zero(fifo.buf + fifo.w);

#if (DAC7554_SOFTSPI == 0)
  lock_spi = 0;
#endif

  if (nrf24l01p_cmd_len == 0) return ;

  fifo.w = fifo_mod(fifo.w + NRF24L01P_PAYLOAD_WIDTH);
}


/* main */

int main(void)
{
  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

  uart_setup();

  nrf24l01p_setup();

  /* sparkfun usb serial board configuration */
  /* NOTE: nrf24l01p_enable_crc8(); for nrf24l01p board */
  /* nrf24l01p_enable_crc16(); */
  nrf24l01p_disable_crc();
  /* auto ack disabled */
  /* auto retransmit disabled */
  /* 4 bytes payload */
  /* 1mbps, 0dbm */
  /* nrf24l01p_set_rate(NRF24L01P_RATE_1MBPS); */
  nrf24l01p_set_rate(NRF24L01P_RATE_2MBPS);
  /* nrf24l01p_set_rate(NRF24L01P_RATE_250KBPS); */
  /* channel 2 */
  nrf24l01p_set_chan(2);
  /* 5 bytes addr width */
  /* nrf24l01p_set_addr_width(NRF24L01P_ADDR_WIDTH_5); */
  nrf24l01p_set_addr_width(NRF24L01P_ADDR_WIDTH_3);
  /* rx address */
  nrf24l01p_cmd_buf[0] = 0xe7;
  nrf24l01p_cmd_buf[1] = 0xe7;
  nrf24l01p_cmd_buf[2] = 0xe7;
  nrf24l01p_cmd_buf[3] = 0xe7;
  nrf24l01p_cmd_buf[4] = 0xe7;
  nrf24l01p_write_reg40(NRF24L01P_REG_RX_ADDR_P0);
  /* tx address */
  nrf24l01p_cmd_buf[0] = 0xe7;
  nrf24l01p_cmd_buf[1] = 0xe7;
  nrf24l01p_cmd_buf[2] = 0xe7;
  nrf24l01p_cmd_buf[3] = 0xe7;
  nrf24l01p_cmd_buf[4] = 0xe7;
  nrf24l01p_write_reg40(NRF24L01P_REG_TX_ADDR);
  /* enable tx no ack command */
  nrf24l01p_enable_tx_noack();

  nrf24l01p_powerdown_to_standby();

  fifo_setup();
  dac7554_setup();

  /* setup timer1, ctc mode, interrupt on match 400 */
  TCNT1 = 0;
  OCR1A = 400;
  OCR1B = 0xffff;
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1C = 0;
  TIMSK1 = 1 << 1;

  /* enable interrupts */
  sei();

  uart_write((uint8_t*)"rx side\r\n", 9);
  uart_write((uint8_t*)"press space\r\n", 13);
  uart_read_uint8();
  uart_write((uint8_t*)"starting\r\n", 10);

  nrf24l01p_standby_to_rx();

  if (nrf24l01p_is_rx_full()) nrf24l01p_flush_rx();

  while (fifo.w < (sizeof(fifo.buf) / 2))
  {
    /* wait for irq and process */
    while (nrf24l01p_is_rx_irq() == 0) ;
    on_nrf24l01p_irq();
    /* still in rx mode */
  }

  /* clock source disabled, safe to access 16 bits counter */
  TCNT1 = 0;
  /* prescaler set to 1, interrupt at 40khz */
  TCCR1B = (1 << 3) | (1 << 0);

  while (1)
  {
    if (nrf24l01p_is_rx_irq())
    {
      on_nrf24l01p_irq();
    }
    else if (nrf24l01p_is_rx_full())
    {
      nrf24l01p_flush_rx();
    }
#if 0 /* TODO */
    else if (fifo_is_empty())
    {
      /* disable timer1 interrupts */
    }
#endif /* TODO */
  }

  return 0;
}
