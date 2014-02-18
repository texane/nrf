#include <stdint.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#define NRF_CONFIG_NRF905 1
#include "../../../../src/nrf.c"


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
#define PAYLOAD_WIDTH 16
  static uint8_t tx_buf[PAYLOAD_WIDTH];
  uint8_t i;

  for (i = 0; i < PAYLOAD_WIDTH; ++i) tx_buf[i] = tx_pattern;
  ++tx_pattern;

#if (NRF_CONFIG_NRF24L01P == 1)

  /* assumed was in rx mode */
  nrf24l01p_rx_to_tx();

  nrf24l01p_write_tx_noack_zero(tx_buf);
  nrf24l01p_complete_tx_noack_zero();
  while (nrf24l01p_is_tx_irq() == 0) ;

  nrf24l01p_tx_to_rx();

#elif (NRF_CONFIG_NRF905 == 1)

  nrf905_write_payload_zero(tx_buf);
  nrf905_set_rx();

#endif
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

  uint8_t* buf;

  if (nrf_get_rx_irq() == 0) return 0;

  nrf_read_payload(&buf);

  /* set pattern */
  if (buf[0] == '0')
  {
    /* set tx pattern */
    tx_pattern = hex_to_uint8(buf + 1);
  }
  else if (buf[0] == '1')
  {
    /* set tx frequency */
    const uint8_t x = hex_to_uint8(buf + 1);
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

ISR(PCINT2_vect)
{
  /* pin change 2 interrupt handler */
  /* do not handle here, otherwise compl */
  /* msg may get mixed with nrf payload */
}

int main(void)
{
  nrf_setup();
  nrf_set_powerdown_mode();

  /* setup portd3 interrupt on change. disable pullup. */
  DDRD &= ~(1 << 3);
  PORTD &= ~(1 << 3);
  PCICR |= 1 << 2;
  /* enable portb1 interrupt on change */
  PCMSK2 |= 1 << 3;

  /* disable timer */
  timer_disable();
  /* timer_count_hi = 1; */
  /* timer_enable(); */

  /* timer and pinchange int wakeup sources */
  set_sleep_mode(SLEEP_MODE_IDLE);

  nrf_set_rx_mode();

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
