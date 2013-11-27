#include <stdint.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "../../../../src/spi.c"
#include "../../../../src/nrf24l01p.c"

/* timer */

static volatile uint8_t timer_irq = 0;
static volatile uint8_t timer_count_hi = 0;
static volatile uint8_t timer_count = 0;

ISR(TIMER1_COMPA_vect)
{
  if ((++timer_count) == timer_count_hi)
  {
    timer_irq = 1;
    timer_count = 0;
  }
}

static void timer_enable(void)
{
  /* 16 bits timer1 is used */
  /* interrupt at TIMER_FREQ hz */
  /* fcpu / (1024 * 15625) = 1 hz */

  /* stop timer */
  TCCR1B = 0;

  /* CTC mode, overflow when OCR1A reached */
  TCCR1A = 0;
  OCR1A = 15625;
  TCNT1 = 0;
  TCCR1C = 0;

  /* interrupt on OCIE0A match */
  TIMSK1 = 1 << 1;

  /* start timer */
  /* prescaler set to 1024 */
  TCCR1B = (1 << 3) | (5 << 0);
}

static void timer_disable(void)
{
  TCCR1B = 0;
}

/* nrf tx rx logic */

static uint8_t tx_pattern = 0x00;

static void do_tx(void)
{
  static uint8_t tx_buf[NRF24L01P_PAYLOAD_WIDTH];

  uint8_t i;

  for (i = 0; i < NRF24L01P_PAYLOAD_WIDTH; ++i)
    tx_buf[i] = tx_pattern;
  ++tx_pattern;

  /* assumed was in rx mode */
  nrf24l01p_rx_to_tx();

  nrf24l01p_write_tx_noack_zero(tx_buf);
  nrf24l01p_complete_tx_noack_zero();
  while (nrf24l01p_is_tx_irq() == 0) ;

  nrf24l01p_tx_to_rx();
}

static uint8_t nibble_to_uint8(uint8_t c)
{
  if ((c >= 'a') && (c <= 'f')) return 0xa + c - 'a';
  return 0x0 + c - '0';
}

static uint8_t hex_to_uint8(const uint8_t* s)
{
  return (nibble_to_uint8(s[0]) << 4) | nibble_to_uint8(s[1]);
}

static uint8_t do_rx(void)
{
  /* return 0 if no msg processed, 1 otherwise */

  if (nrf24l01p_is_rx_irq() == 0) return 0;

  nrf24l01p_read_rx();

  /* set pattern */
  if (nrf24l01p_cmd_buf[0] == '0')
  {
    /* set tx pattern */
    tx_pattern = hex_to_uint8(nrf24l01p_cmd_buf + 1);
  }
  else if (nrf24l01p_cmd_buf[0] == '1')
  {
    /* set tx frequency */
    const uint8_t x = hex_to_uint8(nrf24l01p_cmd_buf + 1);
    if (x == 0)
    {
      timer_disable();
    }
    else
    {
      timer_count_hi = x;
      timer_count = 0;
      timer_enable();
    }
  }
  /* else */

  /* on message handled */
  return 1;
}

ISR(PCINT0_vect)
{
  /* pin change 0 interrupt handler */
  /* do not handle here, otherwise compl */
  /* msg may get mixed with nrf payload */
}

int main(void)
{
  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

  /* nrf24l01p default configuration */

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
  nrf24l01p_set_addr_width(NRF24L01P_ADDR_WIDTH_4);
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

  nrf24l01p_set_powerdown();

  /* setup interrupt on change. disable pullup. */
  DDRB &= ~(1 << 1);
  PORTB &= ~(1 << 1);
  PCICR |= 1 << 0;
  /* enable portb1 interrupt on change */
  PCMSK0 |= 1 << 1;

  /* disable timer */
  timer_disable();
  /* timer_count_hi = 1; */
  /* timer_enable(); */

  /* timer and pinchange int wakeup sources */
  set_sleep_mode(SLEEP_MODE_IDLE);

  nrf24l01p_powerdown_to_standby();
  nrf24l01p_standby_to_rx();

  while (1)
  {
    sleep_disable();
    cli();

    if (timer_irq)
    {
      timer_irq = 0;
      do_tx();
    }

    while (do_rx()) ;

    sleep_enable();
    sleep_bod_disable();
    /* atomic, no int schedule between sei and sleep_cpu */
    sei();
    sleep_cpu();
  }

  return 0;
}
