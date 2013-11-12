#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include "../common/snrf_common.h"
#include "../../../common/nrf24l01p.c"
#include "../../../common/uart.c"


/* snrf global variables */

static uint8_t snrf_state;


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

static uint32_t buf_to_uint32(const uint8_t* buf)
{
  uint32_t x = 0;

  x |= ((uint32_t)buf[0]) << 24;
  x |= ((uint32_t)buf[1]) << 16;
  x |= ((uint32_t)buf[2]) << 8;
  x |= ((uint32_t)buf[3]) << 0;

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
  MAKE_COMPL_ERROR(msg, SNRF_ERR_SUCCESS);

  switch (key)
  {
  case SNRF_KEY_STATE:
    {
      if (val >= SNRF_STATE_MAX)
      {
	MAKE_COMPL_ERROR(msg, SNRF_ERR_VAL);
	break ;
      }

      if (val == snrf_state)
      {
	/* nothing */
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
    if (val == SNRF_CRC_DISABLED) nrf24l01p_disable_crc();
    else if (val == SNRF_CRC_8) nrf24l01p_enable_crc8();
    else if (val == SNRF_CRC_16) nrf24l01p_enable_crc16();
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
      else nrf24l01p_set_addr_width(map[val]);
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
  MAKE_COMPL_ERROR(msg, SNRF_ERR_SUCCESS);

  switch (key)
  {
  case SNRF_KEY_STATE:
    msg->u.compl.val = uint32_to_le(snrf_state);
    break ;

  case SNRF_KEY_CRC:
    {
      const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_CONFIG);
      if ((x & (1 << 3)) == 0) msg->u.compl.val = uint32_to_le(0);
      else if ((x & (1 << 2)) == 0) msg->u.compl.val = uint32_to_le(1);
      else msg->u.compl.val = uint32_to_le(2);
      break ;
    }

  case SNRF_KEY_RATE:
    {
      const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_RF_SETUP);
      static const uint8_t map[] =
      {
	NRF24L01P_RATE_1MBPS,
	NRF24L01P_RATE_2MBPS,
	NRF24L01P_RATE_250KBPS,
	NRF24L01P_RATE_250KBPS,
	NRF24L01P_RATE_250KBPS
      };
      msg->u.compl.val = uint32_to_le(map[(x >> 3) & 5]);
      break ;
    }

  case SNRF_KEY_CHAN:
    {
      const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_RF_CH);
      msg->u.compl.val = uint32_to_le(x);
      break ;
    }

  case SNRF_KEY_ADDR_WIDTH:
    {
      static const uint8_t map[] =
      {
	0xff, /* invalid */
	0, /* width = 3 */
	1, /* width = 4 */
	2 /* width = 5 */
      };

      const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_SETUP_AW);
      msg->u.compl.val = uint32_to_le(map[x & 3]);
      break ;
    }

  case SNRF_KEY_RX_ADDR:
    {
      nrf24l01p_read_reg40(NRF24L01P_REG_RX_ADDR_P0);
      msg->u.compl.val = uint32_to_le(buf_to_uint32(nrf24l01p_cmd_buf));
      break ;
    }

  case SNRF_KEY_TX_ADDR:
    {
      nrf24l01p_read_reg40(NRF24L01P_REG_TX_ADDR);
      msg->u.compl.val = uint32_to_le(buf_to_uint32(nrf24l01p_cmd_buf));
      break ;
    }

  case SNRF_KEY_TX_ACK:
    {
      const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_FEATURE);
      if (x & (1 << 0)) msg->u.compl.val = uint32_to_le(0);
      else msg->u.compl.val = uint32_to_le(1);
      break ;
    }

  case SNRF_KEY_PAYLOAD_WIDTH:
    {
      const uint8_t x = nrf24l01p_read_reg8(NRF24L01P_REG_RX_PW_P0);
      msg->u.compl.val = uint32_to_le(x);
      break ;
    }

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
  MAKE_COMPL_ERROR(msg, SNRF_ERR_SUCCESS);
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

#if 0 /* unused */

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

#endif /* unused */

static uint8_t do_nrf(void)
{
  /* return 0 if no msg processed, 1 otherwise */

  static snrf_msg_t msg;
  uint8_t i;
  uint8_t size;

  if (nrf24l01p_is_rx_irq() == 0) return 0;

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

  /* on message handled */
  return 1;
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

static volatile uint8_t uart_buf[sizeof(snrf_msg_t)];
static volatile uint8_t uart_pos = 0;

#define UART_FLAG_MISS (1 << 0)
#define UART_FLAG_ERR (1 << 1)
static volatile uint8_t uart_flags = 0;

ISR(USART_RX_vect)
{
  /* warning: uart_pos must only be incremented. this allows */
  /* the sequential part of the code to access its contents */
  /* without disabling uart interrupts. especially, uart_pos */
  /* must not be set to 0 here on error, and doing so is left */
  /* to the sequential */
  
  while (uart_is_rx_empty() == 0)
  {
    /* missed byte */
    if (uart_pos == sizeof(snrf_msg_t))
    {
      uint8_t x;
      uart_read_uint8(&x);
      uart_flags |= UART_FLAG_MISS;
      return ;
    }

    /* uart rx error */
    if (uart_read_uint8((uint8_t*)&uart_buf[uart_pos]))
    {
      uart_flags |= UART_FLAG_ERR;
      return ;
    }

    ++uart_pos;
  }
}

static uint8_t do_uart(void)
{
  /* return 0 if no msg processed, 1 otherwise */

  /* note: commands comming from the host always */
  /* require a form of ack, which implies that */
  /* there wont be a uart rx burst from the dev */
  /* point of view. thus, this routine can be */
  /* slow to handle message without missing */
  /* bytes, until the reply is sent. */

  /* no need to disable interrupts. cf USART_RX_vect comment. */
  const uint8_t pos = uart_pos;

  /* no need for atomic access. */
  const uint8_t flags = uart_flags;

  uint8_t x;

  if (flags & UART_FLAG_ERR)
  {
    /* TODO: send error to host. */
    uart_flags &= ~UART_FLAG_ERR;
    uart_pos = 0;
    return 0;
  }
  else if (flags & UART_FLAG_MISS)
  {
    /* TODO: send error to host */
    /* not an error, continue */
    uart_flags &= ~UART_FLAG_MISS;
  }

  if (pos != sizeof(snrf_msg_t))
  {
    /* not a full message available */
    return 0;
  }

  /* synchronization procedure */
  if (uart_buf[offsetof(snrf_msg_t, sync)] == SNRF_SYNC_BYTE)
  {
    while (1)
    {
      /* do not stop on error during sync */
      if (uart_read_uint8(&x)) continue ;
      if (x == SNRF_SYNC_END) break ;
    }

    uart_pos = 0;

    /* set state to conf */
    if (snrf_state != SNRF_STATE_CONF)
    {
      snrf_state = SNRF_STATE_CONF;
      nrf24l01p_set_powerdown();
    }

    return 1;
  }

  /* handle new message */
  handle_msg((snrf_msg_t*)uart_buf);

  /* warning: set uart_pos to 0 before sending */
  /* the reply, since the host will start sending */
  /* new packets and we do not want the interrupt */
  /* handler to see uart_buf full */
  uart_pos = 0;

  /* send completion */
  uart_write((uint8_t*)uart_buf, sizeof(uart_buf));

  /* a message has been handled */
  return 1;
}

/* main */

int main(void)
{
  /* default state to conf */
  snrf_state = SNRF_STATE_CONF;

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

    /* alternate do_{uart,nrf} to avoid starvation */
    while (1)
    {
      uint8_t is_msg = do_uart();
      if (snrf_state != SNRF_STATE_CONF) is_msg |= do_nrf();
      if (is_msg == 0) break ;
    }

    sleep_enable();
    sleep_bod_disable();
    /* atomic, no int schedule between sei and sleep_cpu */
    sei();
    sleep_cpu();
  }

  return 0;
}
