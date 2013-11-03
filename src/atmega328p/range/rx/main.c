#include <stdint.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "../../common/nrf24l01p.c"


#define CONFIG_USE_UART 0
#if (CONFIG_USE_UART == 1)
#include "../../common/uart.c"
#endif

#define CONFIG_USE_PCINT 1
#define CONFIG_USE_SLEEP 1
#define CONFIG_USE_LED 1


static void set_led(uint8_t mask)
{
#define LED_RX_POS 6
#define LED_MATCH_POS 7
#define LED_RX_MASK (1 << LED_RX_POS)
#define LED_MATCH_MASK (1 << LED_MATCH_POS)

  static uint8_t pre_mask = 0;

  DDRD |= (LED_RX_MASK | LED_MATCH_MASK);

  if (mask == pre_mask) return ;

  PORTD &= ~(LED_RX_MASK | LED_MATCH_MASK);
#if (CONFIG_USE_LED == 1)
  PORTD |= mask;
#endif /* CONFIG_USE_LED */

  pre_mask = mask;
}

static int compare_pattern(void)
{
  uint8_t i;
  uint8_t x;

  if (nrf24l01p_cmd_len < NRF24L01P_PAYLOAD_WIDTH)
    return -1;

  for (i = 0, x = 0x2a; i < NRF24L01P_PAYLOAD_WIDTH; ++i, ++x)
  {
    if (nrf24l01p_cmd_buf[i] != x) return -1;
  }

  return 0;
}

static inline uint8_t on_nrf24l01p_irq(void)
{
  uint8_t mask = 0;

  nrf24l01p_read_rx();

  if (nrf24l01p_cmd_len)
  {
    mask |= LED_RX_MASK;
    if (compare_pattern() == 0) mask |= LED_MATCH_MASK;
  }

  return mask;
}


#if (CONFIG_USE_PCINT == 1)

static volatile uint8_t is_irq = 0;

ISR(PCINT0_vect)
{
  /* pin change 0 interrupt handler */
  if (nrf24l01p_is_rx_irq() == 0) return ;
  is_irq = 1;
}

#endif /* CONFIG_USE_PCINT */


/* main */

int main(void)
{
  uint32_t counter[2] = {0, 0};

  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

#if (CONFIG_USE_UART == 1)
  uart_setup();
#endif

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

#if (CONFIG_USE_PCINT == 1)
  /* setup interrupt on change. disable pullup. */
  DDRB &= ~(1 << 1);
  PORTB &= ~(1 << 1);
  PCICR |= 1 << 0;
  /* enable portb1 interrupt on change */
  PCMSK0 |= 1 << 1;
#endif /* CONFIG_USE_PCINT */

  /* enable interrupts */
  sei();

#if (CONFIG_USE_UART == 1)
  uart_write((uint8_t*)"rx side\r\n", 9);
  uart_write((uint8_t*)"press space\r\n", 13);
  uart_read_uint8();
  uart_write((uint8_t*)"starting\r\n", 10);
#endif

  set_led(0);

  nrf24l01p_standby_to_rx();

  if (nrf24l01p_is_rx_full()) nrf24l01p_flush_rx();

#if (CONFIG_USE_SLEEP == 1)
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
#endif

  while (1)
  {
#if (CONFIG_USE_PCINT == 0)
    const uint8_t is_irq = nrf24l01p_is_rx_irq();
#endif

#if (CONFIG_USE_SLEEP == 1)
    /* todo: non atomic, there should be a timer interrupt */
    if (is_irq == 0)
    {
      sleep_enable();
      sleep_cpu();
      sleep_disable();
    }
#endif

    if (is_irq)
    {
#if (CONFIG_USE_PCINT == 1)
      is_irq = 0;
#endif

      const uint8_t led_mask = on_nrf24l01p_irq();
      if (led_mask & LED_RX_MASK) counter[0] = 0;
      if (led_mask & LED_MATCH_MASK) counter[1] = 0;
      set_led(led_mask);
    }
    else if (nrf24l01p_is_rx_full())
    {
      nrf24l01p_flush_rx();
    }

    ++counter[0];
    ++counter[1];

    if (counter[0] == 200000UL)
    {
      counter[0] = 0;
      set_led(0);
    }

    if (counter[1] == 200000UL)
    {
      counter[1] = 0;
      set_led(0);
    }
  }

  return 0;
}
