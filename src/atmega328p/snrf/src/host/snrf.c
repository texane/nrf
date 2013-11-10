#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include "snrf.h"
#include "snrf_common.h"
#include "serial.h"


#define CONFIG_DEBUG 1
#if CONFIG_DEBUG
#include <stdio.h>
#define SNRF_PERROR()					\
do {							\
printf("[!] %s, %u\n", __FILE__, __LINE__);		\
} while (0)
#define SNRF_ASSUME(__x)				\
do {							\
if (!(__x)) printf("[!] %s, %u\n", __FILE__, __LINE__);	\
} while (0)
#else
#define SNRF_PERROR()
#endif


int snrf_open(snrf_handle_t* snrf)
{
  static serial_conf_t conf = { 9600, SERIAL_PARITY_DISABLED, 1 };

  if (serial_open(&snrf->serial, "/dev/ttyUSB0"))
  {
    SNRF_PERROR();
    goto on_error_0;
  }

  if (serial_set_conf(&snrf->serial, &conf))
  {
    SNRF_PERROR();
    goto on_error_1;
  }

  if (snrf_get_keyval(snrf, SNRF_KEY_STATE, &snrf->state))
  {
    SNRF_PERROR();
    goto on_error_1;
  }

  snrf->msg_head = NULL;

  return 0;

 on_error_1:
  snrf_close(snrf);
 on_error_0:
  return -1;
}

int snrf_close(snrf_handle_t* snrf)
{
  serial_close(&snrf->serial);
  return 0;
}

static inline uint32_t uint32_to_le(uint32_t x)
{
  return x;
}

static inline uint32_t le_to_uint32(uint32_t x)
{
  return x;
}

static int write_msg(snrf_handle_t* snrf, snrf_msg_t* msg)
{
  if (serial_writen(&snrf->serial, (const void*)msg, sizeof(*msg)))
  {
    SNRF_PERROR();
    return -1;
  }

  return 0;
}

static int read_msg(snrf_handle_t* snrf, snrf_msg_t* msg)
{
  if (serial_readn(&snrf->serial, (void*)msg, sizeof(*msg)))
  {
    SNRF_PERROR();
    return -1;
  }

  return 0;
}

static int wait_msg(snrf_handle_t* snrf, uint8_t op, snrf_msg_t* msg, size_t n)
{
  snrf_msg_node_t* node;

  for (; n; --n)
  {
    if (read_msg(snrf, msg))
    {
      SNRF_PERROR();
      return -1;
    }

    if (msg->op == op) break ;

    /* put in msg queue */
    node = malloc(sizeof(snrf_msg_node_t));
    memcpy(&node->msg, msg, sizeof(snrf_msg_t));
    node->next = snrf->msg_head;
    snrf->msg_head = node;
  }

  /* n reached */
  return -2;
}

int snrf_write_payload(snrf_handle_t* snrf, const uint8_t* buf, size_t size)
{
  snrf_msg_t msg;

  SNRF_ASSUME(size <= SNRF_MAX_PAYLOAD_WIDTH);

  msg.op = SNRF_OP_PAYLOAD;
  memcpy(msg.u.payload.data, buf, size);
  msg.u.payload.size = (uint8_t)size;

  if (write_msg(snrf, &msg))
  {
    SNRF_PERROR();
    return -1;
  }

  return 0;
}

int snrf_read_payload(snrf_handle_t* snrf, uint8_t* buf, size_t* size)
{
  /* assume buf size <= SNRF_MAX_PAYLOAD_WIDTH */

  snrf_msg_t msg;

  /* look in msg queue */
  if (snrf->msg_head != NULL)
  {
    snrf_msg_node_t* const node = snrf->msg_head;
    snrf->msg_head = node->next;
    memcpy(&msg, &node->msg, sizeof(msg));
    free(node);
  }
  else
  {
    const int err = wait_msg(snrf, SNRF_OP_PAYLOAD, &msg, 1);
    if (err == -1)
    {
      SNRF_PERROR();
      return -1;
    }
    else if (err == -2)
    {
      /* not an error, but do not retry */
      return -2;
    }
  }

  if (msg.u.payload.size > SNRF_MAX_PAYLOAD_WIDTH)
  {
    SNRF_PERROR();
    return -1;
  }

  memcpy(buf, msg.u.payload.data, msg.u.payload.size);
  *size = msg.u.payload.size;

  return 0;
}

int snrf_set_keyval(snrf_handle_t* snrf, uint8_t key, uint32_t val)
{
  snrf_msg_t msg;

  if (key != SNRF_KEY_STATE)
  {
    if (snrf->state != SNRF_STATE_IDLE)
    {
      SNRF_PERROR();
      return -1;
    }
  }

  msg.op = SNRF_OP_SET;
  msg.u.set.key = key;
  msg.u.set.val = uint32_to_le(val);

  if (write_msg(snrf, &msg))
  {
    SNRF_PERROR();
    return -1;
  }

  if (wait_msg(snrf, SNRF_OP_COMPL, &msg, (size_t)-1))
  {
    SNRF_PERROR();
    return -1;
  }

  if (msg.u.compl.err)
  {
    SNRF_PERROR();
    return -1;
  }

  return 0;
}

int snrf_get_keyval(snrf_handle_t* snrf, uint8_t key, uint32_t* val)
{
  snrf_msg_t msg;

  msg.op = SNRF_OP_GET;
  msg.u.set.key = key;

  if (write_msg(snrf, &msg))
  {
    SNRF_PERROR();
    return -1;
  }

  if (wait_msg(snrf, SNRF_OP_COMPL, &msg, (size_t)-1))
  {
    SNRF_PERROR();
    return -1;
  }

  if (msg.u.compl.err)
  {
    SNRF_PERROR();
    return -1;
  }

  *val = le_to_uint32(msg.u.compl.val);

  return 0;
}
