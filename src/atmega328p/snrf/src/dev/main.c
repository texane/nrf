#include <stdint.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "../common/snrf_common.h"
#include "../../../common/nrf24l01p.c"
#include "../../../common/uart.c"


/* snrf global variables */

static uint8_t snrf_state = SNRF_STATE_CONF;


/* message handlers */

#define MAKE_COMPL_ERROR(__m, __e)		\
do {						\
  (__m)->op = SNRF_OP_COMPL;			\
  (__m)->u.compl.err = __e;			\
  (__m)->u.compl.val = __LINE__;		\
} while (0)

#define ARRAY_COUNT(__a) (sizeof(__a) / sizeof((__a)[0]))

static inline uint32_t uint32_to_le(uint32_t x)
{
  return x;
}

static inline uint32_t le_to_uint32(uint32_t x)
{
  return x;
}

static void handle_set_msg(snrf_msg_t* msg)
{
  /* capture before modifying */
  const uint8_t key = msg->u.set.key;
  const uint32_t val = le_to_uint32(msg->u.set.val);

  if ((snrf_state != SNRF_STATE_CONF) && (key != SNRF_KEY_STATE))
  {
    MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
    return ;
  }

  /* completion sent back, assume success */
  msg->op = SNRF_OP_COMPL;
  msg->u.compl.err = 0;

  switch (key)
  {
  case SNRF_KEY_STATE:
    {
      if (val >= SNRF_STATE_MAX)
      {
	MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
	break ;
      }

      if (val == SNRF_STATE_CONF)
      {
	nrf24l01p_set_powerdown();
      }
      else if (val == SNRF_STATE_TXRX)
      {
	nrf24l01p_powerdown_to_standby();
	nrf24l01p_standby_to_rx();
      }

      snrf_state = val;

      break ;
    }

  case SNRF_KEY_CRC:
    if (val == 0) nrf24l01p_disable_crc();
    else if (val == 1) nrf24l01p_enable_crc8();
    else if (val == 2) nrf24l01p_enable_crc16();
    else MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
    break ;

  case SNRF_KEY_RATE:
    {
      static const uint8_t map[] =
      {
	NRF24L01P_RATE_250KBPS,
	NRF24L01P_RATE_1MBPS,
	NRF24L01P_RATE_2MBPS
      };
      if (val >= ARRAY_COUNT(map)) MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
      else nrf24l01p_set_rate(map[val]);
      break ;
    }

  case SNRF_KEY_CHAN:
    nrf24l01p_set_chan((uint8_t)val);
    break ;

  case SNRF_KEY_ADDR_WIDTH:
    {
      static const uint8_t map[] =
      {
	NRF24L01P_ADDR_WIDTH_3,
	NRF24L01P_ADDR_WIDTH_4
#if 0
	/* not supported, value limited to 32 bits */
	NRF24L01P_ADDR_WIDTH_5
#endif
      };
      if (val >= ARRAY_COUNT(map)) MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
      else nrf24l01p_set_rate(map[val]);
      break ;
    }

  case SNRF_KEY_RX_ADDR:
    nrf24l01p_cmd_buf[0] = (uint8_t)((val >> 24) & 0xff);
    nrf24l01p_cmd_buf[1] = (uint8_t)((val >> 16) & 0xff);
    nrf24l01p_cmd_buf[2] = (uint8_t)((val >> 8) & 0xff);
    nrf24l01p_cmd_buf[3] = (uint8_t)((val >> 0) & 0xff);
    nrf24l01p_cmd_buf[4] = 0x00;
    nrf24l01p_write_reg40(NRF24L01P_REG_RX_ADDR_P0);
    break ;

  case SNRF_KEY_TX_ADDR:
    nrf24l01p_cmd_buf[0] = (uint8_t)((val >> 24) & 0xff);
    nrf24l01p_cmd_buf[1] = (uint8_t)((val >> 16) & 0xff);
    nrf24l01p_cmd_buf[2] = (uint8_t)((val >> 8) & 0xff);
    nrf24l01p_cmd_buf[3] = (uint8_t)((val >> 0) & 0xff);
    nrf24l01p_cmd_buf[4] = 0x00;
    nrf24l01p_write_reg40(NRF24L01P_REG_TX_ADDR);
    break ;

  case SNRF_KEY_TX_ACK:
    /* disable tx ack */
    if (val == 0) nrf24l01p_enable_tx_noack();
    /* enable tx ack */
    else if (val == 1) nrf24l01p_disable_tx_noack();
    else MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
    break ;

  case SNRF_KEY_PAYLOAD_WIDTH:
    if (val > SNRF_MAX_PAYLOAD_WIDTH) MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
    else nrf24l01p_set_payload_width((uint8_t)val);
    break ;

  default:
    MAKE_COMPL_ERROR(msg, SNRF_ERR_KEY);
    break ;
  }
}

static void handle_get_msg(snrf_msg_t* msg)
{
  /* capture before modifying */
  const uint8_t key = msg->u.set.key;

  /* completion sent back, assume success */
  msg->op = SNRF_OP_COMPL;
  msg->u.compl.err = 0;

  switch (key)
  {
  case SNRF_KEY_STATE:
    msg->u.compl.val = uint32_to_le(snrf_state);
    break ;

  default:
    MAKE_COMPL_ERROR(msg, SNRF_ERR_KEY);
    break ;
  }
}

static void handle_payload_msg(snrf_msg_t* msg)
{
  /* send the message */
  /* msg size and payload width assumed equal */

  if (snrf_state != SNRF_STATE_TXRX)
  {
    MAKE_COMPL_ERROR(msg, SNRF_ERR_STATE);
    return ;
  }

  /* assumed was in rx mode */
  nrf24l01p_rx_to_tx();

  nrf24l01p_write_tx_noack_zero(msg->u.payload.data);
  nrf24l01p_complete_tx_noack_zero();
  while (nrf24l01p_is_tx_irq() == 0) ;

  nrf24l01p_tx_to_rx();

  /* fill completion status */
  msg->op = SNRF_OP_COMPL;
  msg->u.compl.err = 0;
}

static void handle_msg(snrf_msg_t* msg)
{
  switch (msg->op)
  {
  case SNRF_OP_SET:
    handle_set_msg(msg);
    break ;

  case SNRF_OP_GET:
    handle_get_msg(msg);
    break ;

  case SNRF_OP_PAYLOAD:
    handle_payload_msg(msg);
    break ;

  default:
    MAKE_COMPL_ERROR(msg, SNRF_ERR_OP);
    break ;
  }
}


/* led, debugging */

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
  PORTD |= mask;

  pre_mask = mask;
}

static void do_nrf(void)
{
  static snrf_msg_t msg;
  uint8_t i;
  uint8_t size;

  while (nrf24l01p_is_rx_irq())
  {
    nrf24l01p_read_rx();

    size = nrf24l01p_cmd_len;
    /* TODO: use an error message */
    if (nrf24l01p_cmd_len > SNRF_MAX_PAYLOAD_WIDTH)
      size = SNRF_MAX_PAYLOAD_WIDTH;

    msg.op = SNRF_OP_PAYLOAD;
    msg.u.payload.size = size;
    for (i = 0; i < size; ++i)
      msg.u.payload.data[i] = nrf24l01p_cmd_buf[i];

    uart_write((uint8_t*)&msg, sizeof(msg));
  }
}

ISR(PCINT0_vect)
{
  /* pin change 0 interrupt handler */
  /* do not handle here, otherwise compl */
  /* msg may get mixed with nrf payload */
}


/* uart interrupt handler */

static void uart_enable_rx_int(void)
{
  UCSR0B |= 1 << RXCIE0;
}

static inline uint8_t uart_is_rx_empty(void)
{
  return (UCSR0A & (1 << 7)) == 0;
}

static void do_uart(void)
{
  static uint8_t uart_buf[sizeof(snrf_msg_t)];
  static uint8_t uart_pos = 0;

  while (uart_is_rx_empty() == 0)
  {
    uart_buf[uart_pos++] = uart_read_uint8();
    if (uart_pos < sizeof(uart_buf)) continue ;

    /* handle new message */
    handle_msg((snrf_msg_t*)uart_buf);

    /* send completion */
    uart_write(uart_buf, sizeof(uart_buf));

    /* pos is modulo buf size */
    uart_pos = 0;
  }
}

ISR(USART_RX_vect)
{
  /* do not handle here, otherwise compl */
  /* msg may get mixed with nrf payload */
}

/* main */

int main(void)
{
  /* setup spi first */
  spi_setup_master();
  spi_set_sck_freq(SPI_SCK_FREQ_FOSC2);

  uart_setup();
  uart_enable_rx_int();

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

  nrf24l01p_set_powerdown();

  /* setup interrupt on change. disable pullup. */
  DDRB &= ~(1 << 1);
  PORTB &= ~(1 << 1);
  PCICR |= 1 << 0;
  /* enable portb1 interrupt on change */
  PCMSK0 |= 1 << 1;

  /* uart and pinchange int wakeup sources */
  set_sleep_mode(SLEEP_MODE_IDLE);

  while (1)
  {
    sleep_disable();
    cli();

    do_uart();
    do_nrf();

    sleep_enable();
    sleep_bod_disable();
    /* atomic, no int schedule between sei and sleep_cpu */
    sei();
    sleep_cpu();
  }

  return 0;
}
