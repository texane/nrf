#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include "snrf.h"


#define PERROR()				\
do {						\
printf("[!] %s, %u\n", __FILE__, __LINE__);	\
} while (0)

static void print_buf(const uint8_t* buf, size_t size)
{
  size_t i;

  for (i = 0; i < size; ++i)
  {
    if (i && ((i % 8) == 0)) printf("\n");
    printf("%02x", buf[i]);
  }
  printf("\n");
}

static uint32_t get_uint32(const char* s)
{
  const size_t len = strlen(s);
  int base;

  if ((len >= 2) && (s[0] == '0') && (s[1] == 'x')) base = 16;
  else base = 10;

  return (uint32_t)strtoul(s, NULL, base);
}

static unsigned int is_digit(const char* s)
{
  return ((s[0] >= '0') && (s[0] <= '9'));
}

static uint8_t str_to_key(const char* s)
{
  if (is_digit(s)) return (uint8_t)get_uint32(s);

#define IF_STREQ_RETURN(__a, __b, __x) \
  if (strcmp(__a, __b) == 0) return SNRF_KEY_ ## __x;

  IF_STREQ_RETURN(s, "info", INFO);
  IF_STREQ_RETURN(s, "state", STATE);
  IF_STREQ_RETURN(s, "crc", CRC);
  IF_STREQ_RETURN(s, "rate", RATE);
  IF_STREQ_RETURN(s, "chan", CHAN);
  IF_STREQ_RETURN(s, "addr_width", ADDR_WIDTH);
  IF_STREQ_RETURN(s, "rx_addr", RX_ADDR);
  IF_STREQ_RETURN(s, "tx_addr", TX_ADDR);
  IF_STREQ_RETURN(s, "tx_ack", TX_ACK);
  IF_STREQ_RETURN(s, "payload_width", PAYLOAD_WIDTH);
  IF_STREQ_RETURN(s, "uart_flags", UART_FLAGS);

  return (uint8_t)-1;
}

static int str_to_keyval
(const char* key_str, const char* val_str, uint8_t* key, uint32_t* val)
{
  *key = str_to_key(key_str);

  if (val_str == NULL)
  {
    *val = (uint32_t)-1;
    if (*key == (uint8_t)-1) return -1;
    return 0;
  }

  switch (*key)
  {
  case SNRF_KEY_STATE:
    if (is_digit(val_str)) goto case_digit;
    else if (strcmp(val_str, "conf") == 0) *val = SNRF_STATE_CONF;
    else if (strcmp(val_str, "txrx") == 0) *val = SNRF_STATE_TXRX;
    else goto on_error;
    break ;

  case SNRF_KEY_CRC:
    if (strcmp(val_str, "8") == 0) *val = SNRF_CRC_8;
    else if (strcmp(val_str, "16") == 0) *val = SNRF_CRC_16;
    else if (strcmp(val_str, "disabled") == 0) *val = SNRF_CRC_DISABLED;
    else goto on_error;
    break ;

  case SNRF_KEY_RATE:
    if (strcmp(val_str, "50KBPS") == 0) *val = SNRF_RATE_50KBPS;
    else if (strcmp(val_str, "250KBPS") == 0) *val = SNRF_RATE_250KBPS;
    else if (strcmp(val_str, "1MBPS") == 0) *val = SNRF_RATE_1MBPS;
    else if (strcmp(val_str, "2MBPS") == 0) *val = SNRF_RATE_2MBPS;
    else if (is_digit(val_str)) goto case_digit;
    else goto on_error;
    break ;

  case SNRF_KEY_TX_ACK:
    if (is_digit(val_str)) goto case_digit;
    else if (strcmp(val_str, "disabled") == 0) *val = SNRF_TX_ACK_DISABLED;
    else if (strcmp(val_str, "enabled") == 0) *val = SNRF_TX_ACK_ENABLED;
    else goto on_error;
    break ;

  case_digit:
  case SNRF_KEY_ADDR_WIDTH:
  case SNRF_KEY_INFO:
  case SNRF_KEY_CHAN:
  case SNRF_KEY_RX_ADDR:
  case SNRF_KEY_TX_ADDR:
  case SNRF_KEY_PAYLOAD_WIDTH:
  case SNRF_KEY_UART_FLAGS:
    *val = get_uint32(val_str);
    break ;

  default:
    goto on_error;
    break ;
  }

  return 0;
 on_error:
  return -1;
}

static void print_keyval(uint8_t key, uint32_t val)
{
  const char* key_str;
  const char* val_str;
  char val_buf[32];

  switch (key)
  {
  case SNRF_KEY_INFO:
    key_str = "info";
    sprintf(val_buf, "0x%08x", val);
    val_str = val_buf;
    break ;

  case SNRF_KEY_STATE:
    key_str = "state";
    if (val == SNRF_STATE_CONF) val_str = "conf";
    else if (val == SNRF_STATE_CONF) val_str = "conf";
    else val_str = "invalid";
    break ;

  case SNRF_KEY_CRC:
    key_str = "crc";
    if (val == SNRF_CRC_DISABLED) val_str = "disabled";
    else if (val == SNRF_CRC_8) val_str = "8";
    else if (val == SNRF_CRC_16) val_str = "16";
    else val_str = "invalid";
    break ;

  case SNRF_KEY_RATE:
    key_str = "rate";
    if (val == SNRF_RATE_50KBPS) val_str = "50KBPS";
    else if (val == SNRF_RATE_250KBPS) val_str = "250KBPS";
    else if (val == SNRF_RATE_1MBPS) val_str = "1MBPS";
    else if (val == SNRF_RATE_2MBPS) val_str = "2MBPS";
    else val_str = "invalid";
    break ;

  case SNRF_KEY_CHAN:
    key_str = "chan";
    sprintf(val_buf, "%u", val);
    val_str = val_buf;
    break ;

  case SNRF_KEY_ADDR_WIDTH:
    key_str = "addr_width";
    sprintf(val_buf, "%u", val);
    val_str = val_buf;
    break ;

  case SNRF_KEY_RX_ADDR:
    key_str = "rx_addr";
    sprintf(val_buf, "0x%08x", val);
    val_str = val_buf;
    break ;

  case SNRF_KEY_TX_ADDR:
    key_str = "tx_addr";
    sprintf(val_buf, "0x%08x", val);
    val_str = val_buf;
    break ;

  case SNRF_KEY_TX_ACK:
    key_str = "tx_ack";
    if (val == SNRF_TX_ACK_DISABLED) val_str = "disabled";
    else if (val == SNRF_TX_ACK_ENABLED) val_str = "enabled";
    else val_str = "invalid";
    break ;

  case SNRF_KEY_PAYLOAD_WIDTH:
    key_str = "payload_width";
    sprintf(val_buf, "%u", val);
    val_str = val_buf;
    break ;

  case SNRF_KEY_UART_FLAGS:
    key_str = "uart_flags";
    sprintf(val_buf, "0x%02x", val);
    val_str = val_buf;
    break ;

  default:
    key_str = "invalid";
    val_str = "invalid";
    break ;
  }

  printf("%s = %s\n", key_str, val_str);
}

int main(int ac, char** av)
{
  const char* const op = av[1];
  snrf_handle_t snrf;
  int err = -1;

  if (snrf_open(&snrf))
  {
    PERROR();
    goto on_error_0;
  }

  if (strcmp(op, "read") == 0)
  {
    /* read one payload */

    const int fd = snrf_get_fd(&snrf);

    uint8_t buf[SNRF_MAX_PAYLOAD_WIDTH];
    size_t count;
    size_t size;
    fd_set set;

    if (snrf_set_keyval(&snrf, SNRF_KEY_STATE, SNRF_STATE_TXRX))
    {
      PERROR();
      goto on_error_1;
    }

    if (ac > 2) count = (size_t)get_uint32(av[2]);
    else count = (size_t)-1;

    while (count)
    {
      memset(buf, 0x2a, sizeof(buf));

      FD_ZERO(&set);
      FD_SET(fd, &set);

      if (select(fd + 1, &set, NULL, NULL, NULL) <= 0)
      {
	PERROR();
	goto on_error_1;
      }

      if (snrf_read_payload(&snrf, buf, &size))
      {
	PERROR();
	goto on_error_1;
      }

      print_buf(buf, size);

      if (count != (size_t)-1) --count;
    }
  }
  else if (strcmp(op, "write") == 0)
  {
    /* write one payload */

    uint8_t buf[SNRF_MAX_PAYLOAD_WIDTH];
    size_t size;

    if (snrf_set_keyval(&snrf, SNRF_KEY_STATE, SNRF_STATE_TXRX))
    {
      PERROR();
      goto on_error_1;
    }

    size = strlen(av[2]);
    if (size > SNRF_MAX_PAYLOAD_WIDTH) size = SNRF_MAX_PAYLOAD_WIDTH;
    memcpy(buf, av[2], size);

    if (snrf_write_payload(&snrf, buf, size))
    {
      PERROR();
      goto on_error_1;
    }
  }
  else if (strcmp(op, "set") == 0)
  {
    uint8_t key;
    uint32_t val;

    if (str_to_keyval(av[2], av[3], &key, &val))
    {
      PERROR();
      goto on_error_1;
    }

    if (snrf_set_keyval(&snrf, SNRF_KEY_STATE, SNRF_STATE_CONF))
    {
      PERROR();
      goto on_error_1;
    }

    if (snrf_set_keyval(&snrf, key, val))
    {
      PERROR();
      goto on_error_1;
    }    
  }
  else if (strcmp(op, "get") == 0)
  {
    uint8_t key;
    uint32_t val;

    if (str_to_keyval(av[2], NULL, &key, &val))
    {
      PERROR();
      goto on_error_1;
    }

    if (snrf_set_keyval(&snrf, SNRF_KEY_STATE, SNRF_STATE_CONF))
    {
      PERROR();
      goto on_error_1;
    }

    if (snrf_get_keyval(&snrf, key, &val))
    {
      PERROR();
      goto on_error_1;
    }

    print_keyval(key, val);
  }
  else
  {
    PERROR();
    goto on_error_1;
  }

  /* success */
  err = 0;

 on_error_1:
  snrf_close(&snrf);
 on_error_0:
  return err;
}
