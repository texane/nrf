#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
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
  static const serial_conf_t conf = { 9600, 8, SERIAL_PARITY_DISABLED, 1 };

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

  /* synchronize data streams */
  if (snrf_sync(snrf))
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
  snrf->msg_tail = NULL;

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

static int select_read(int fd, struct timeval* tm)
{
  /* ms the timeout in milliseconds */

  struct timeval tm_start;
  struct timeval tm_stop;
  struct timeval tm_diff;
  fd_set set;
  int err;

  FD_ZERO(&set);
  FD_SET(fd, &set);

  gettimeofday(&tm_start, NULL);
  err = select(fd + 1, &set, NULL, NULL, tm);
  gettimeofday(&tm_stop, NULL);

  /* update timer */
  if (tm != NULL)
  {
    timersub(&tm_stop, &tm_start, &tm_diff);
    /* use tm_start as a tmp result */
    timersub(tm, &tm_diff, &tm_start);
    tm->tv_sec = tm_start.tv_sec;
    tm->tv_usec = tm_start.tv_usec;
  }

  return err;
}

static int read_msg(snrf_handle_t* snrf, snrf_msg_t* msg, struct timeval* tm)
{
  uint8_t* buf;
  size_t size;
  size_t nread;
  int err;

  buf = (uint8_t*)msg;
  size = sizeof(snrf_msg_t);

  while (size)
  {
    err = select_read(serial_get_fd(&snrf->serial), tm);
    if (err < 0)
    {
      SNRF_PERROR();
      return -1;
    }
    else if (err == 0)
    {
      /* timeout */
      return -2;
    }
    /* else */

    if (serial_read(&snrf->serial, buf, size, &nread))
    {
      SNRF_PERROR();
      return -1;
    }

    buf += nread;
    size -= nread;
  }

  return 0;
}

static int wait_msg
(snrf_handle_t* snrf, uint8_t op, snrf_msg_t* msg, unsigned int ms)
{
  /* ms the timeout in milliseconds or -1 */

  snrf_msg_node_t* node;
  int err;
  struct timeval tm;
  struct timeval* p;

  p = NULL;
  if (ms != (unsigned int)-1)
  {
    tm.tv_sec = ms / 1000;
    tm.tv_usec = (ms % 1000) * 1000;
    p = &tm;
  }

  while (1)
  {
    err = read_msg(snrf, msg, p);

    if (err < 0)
    {
      SNRF_PERROR();
      return err;
    }

    if (msg->op == op)
    {
      return 0;
    }

    /* put in msg queue, append at tail */
    node = malloc(sizeof(snrf_msg_node_t));
    memcpy(&node->msg, msg, sizeof(snrf_msg_t));
    node->next = NULL;
    if (snrf->msg_head == NULL) snrf->msg_head = node;
    else snrf->msg_tail->next = node;
    snrf->msg_tail = node;
  }

  /* not reached */
  return 0;
}

static int write_wait_msg(snrf_handle_t* snrf, snrf_msg_t* msg)
{
  /* resynchronize if needed */

  snrf_msg_t saved_msg;
  unsigned int n = 0;
  int err;

  memcpy(&saved_msg, msg, sizeof(snrf_msg_t));

 redo_msg:
  if ((++n) == 4)
  {
    SNRF_PERROR();
    return -1;
  }

  if (write_msg(snrf, msg))
  {
    SNRF_PERROR();
    return -1;
  }

  err = wait_msg(snrf, SNRF_OP_COMPL, msg, 1000);
  if (err == -1)
  {
    SNRF_PERROR();
    return -1;
  }
  else if (err == -2)
  {
    if (snrf_sync(snrf))
    {
      SNRF_PERROR();
      return -1;
    }
    memcpy(msg, &saved_msg, sizeof(snrf_msg_t));
    goto redo_msg;
  }

  return 0;
}

int snrf_write_payload(snrf_handle_t* snrf, const uint8_t* buf, size_t size)
{
  snrf_msg_t msg;

  SNRF_ASSUME(size <= SNRF_MAX_PAYLOAD_WIDTH);

  msg.op = SNRF_OP_PAYLOAD;
  memcpy(msg.u.payload.data, buf, size);
  msg.u.payload.size = (uint8_t)size;
  msg.sync = 0x00;

  if (write_wait_msg(snrf, &msg))
  {
    SNRF_PERROR();
    return -1;
  }

  if (msg.u.compl.err != SNRF_ERR_SUCCESS)
  {
    SNRF_PERROR();
    return -1;
  }

  return 0;
}

int snrf_read_payload(snrf_handle_t* snrf, uint8_t* buf, size_t* size)
{
  /* assume buf size <= SNRF_MAX_PAYLOAD_WIDTH */

  snrf_msg_node_t* node;
  snrf_msg_node_t* prev;
  snrf_msg_t msg;

  /* look in msg queue, start at head */
  node = snrf->msg_head;
  prev = NULL;
  while (node != NULL)
  {
    if (node->msg.op == SNRF_OP_PAYLOAD) break ;
    prev = node;
    node = node->next;
  }

  /* msg found in list */
  if (node != NULL)
  {
    if (node == snrf->msg_head) snrf->msg_head = node->next;
    else prev->next = node->next;
    if (node == snrf->msg_tail) snrf->msg_tail = node->next;
    memcpy(&msg, &node->msg, sizeof(msg));
    free(node);
  }
  else
  {
    const int err = wait_msg(snrf, SNRF_OP_PAYLOAD, &msg, (unsigned int)-1);
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
  /* device must be in conf mode */

  snrf_msg_t msg;

  msg.op = SNRF_OP_SET;
  msg.u.set.key = key;
  msg.u.set.val = uint32_to_le(val);
  msg.sync = 0x00;

  if (write_wait_msg(snrf, &msg))
  {
    SNRF_PERROR();
    return -1;
  }

  if (msg.u.compl.err != SNRF_ERR_SUCCESS)
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
  msg.sync = 0x00;

  if (write_wait_msg(snrf, &msg))
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

int snrf_sync(snrf_handle_t* snrf)
{
  static const uint8_t sync_byte = SNRF_SYNC_BYTE;
  static const uint8_t end_byte = SNRF_SYNC_END;

  size_t i;

  for (i = 0; i < (4 * sizeof(snrf_msg_t)); ++i)
  {
    usleep(1000);
    if (serial_writen(&snrf->serial, &sync_byte, 1))
    {
      SNRF_PERROR();
      return -1;
    }
  }

  usleep(100000);

  if (serial_flush_txrx(&snrf->serial))
  {
    SNRF_PERROR();
    return -1;
  }

  if (serial_writen(&snrf->serial, &end_byte, 1))
  {
    SNRF_PERROR();
    return -1;
  }

  return 0;
}
