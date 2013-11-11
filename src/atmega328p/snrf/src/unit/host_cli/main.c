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
    size_t size;
    fd_set set;

    if (snrf_set_keyval(&snrf, SNRF_KEY_STATE, SNRF_STATE_TXRX))
    {
      PERROR();
      goto on_error_1;
    }

  redo:
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

    goto redo;
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
    const uint8_t key = get_uint32(av[2]);
    const uint32_t val = get_uint32(av[3]);

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
    const uint8_t key = get_uint32(av[2]);
    uint32_t val;

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

    printf("0x%08x\n", val);
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
