#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "serial.h"


#if CONFIG_SERIAL_DEBUG
# define DEBUG_ENTER() printf("[>] %s\n", __FUNCTION__)
# define DEBUG_LEAVE() printf("[<] %s\n", __FUNCTION__)
# define DEBUG_PRINTF(s, ...) printf("[?] %s_%u: " s, __FUNCTION__, __LINE__, ## __VA_ARGS__)
# define DEBUG_ERROR(s, ...) printf("[!] %s_%u: " s, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else /* CONFIG_SERIAL_DEBUG == 0 */
# define DEBUG_ENTER()
# define DEBUG_LEAVE()
# define DEBUG_PRINTF(s, ...)
# define DEBUG_ERROR(s, ...)
#endif /* CONFIG_SERIAL_DEBUG */


static void invalid_handle(serial_handle_t* h)
{
  h->fd = -1;

  memset(&h->termios, 0, sizeof(struct termios));

#if CONFIG_SERIAL_DEBUG
  h->name = NULL;
#endif
}


static speed_t conf_to_speed_t(const serial_conf_t* c)
{
  speed_t s;

  s = 0;

#define CONF_BAUDS_CASE(n) case n: s |= B ## n; break

  switch (c->bauds)
    {
      CONF_BAUDS_CASE(0);
      CONF_BAUDS_CASE(50);
      CONF_BAUDS_CASE(75);
      CONF_BAUDS_CASE(110);
      CONF_BAUDS_CASE(134);
      CONF_BAUDS_CASE(150);
      CONF_BAUDS_CASE(200);
      CONF_BAUDS_CASE(300);
      CONF_BAUDS_CASE(600);
      CONF_BAUDS_CASE(1200);
      CONF_BAUDS_CASE(1800);
      CONF_BAUDS_CASE(2400);
      CONF_BAUDS_CASE(4800);
      CONF_BAUDS_CASE(9600);
      CONF_BAUDS_CASE(19200);
      CONF_BAUDS_CASE(38400);
      CONF_BAUDS_CASE(57600);
      CONF_BAUDS_CASE(115200);

    default:
      goto on_error;
      break;
    }

#define CONF_DATA_CASE(d) case d: s |= CS ## d; break

  switch (c->data)
    {
      CONF_DATA_CASE(5);
      CONF_DATA_CASE(6);
      CONF_DATA_CASE(7);
      CONF_DATA_CASE(8);

    default:
      goto on_error;
      break;
    }

  if (c->stop == 2)
    s |= CSTOPB;

  if (c->parity)
    {
      s |= PARENB;

      if (c->parity == SERIAL_PARITY_ODD)
	s |= PARODD;
    }

  return s;

 on_error:

  return 0;
}


static int speed_t_to_conf(speed_t s, serial_conf_t* c)
{

#define SPEED_BAUDS_CASE(n) case B ## n: c->bauds = n; break

  switch (s & (0x20 - 1))
    {
      SPEED_BAUDS_CASE(50);
      SPEED_BAUDS_CASE(75);
      SPEED_BAUDS_CASE(110);
      SPEED_BAUDS_CASE(134);
      SPEED_BAUDS_CASE(150);
      SPEED_BAUDS_CASE(200);
      SPEED_BAUDS_CASE(300);
      SPEED_BAUDS_CASE(600);
      SPEED_BAUDS_CASE(1200);
      SPEED_BAUDS_CASE(1800);
      SPEED_BAUDS_CASE(2400);
      SPEED_BAUDS_CASE(4800);
      SPEED_BAUDS_CASE(9600);
      SPEED_BAUDS_CASE(19200);
      SPEED_BAUDS_CASE(38400);

    default:

      if ((s & B57600) == B57600)
	c->bauds = 57600;
      else if ((s & B115200) == B115200)
	c->bauds = 115200;
      else
	goto on_error;

      break;
    }

#define SPEED_DATA_CASE(d) case CS ## d: c->data = d; break

  switch (s & (CS5 | CS6 | CS7 | CS8))
    {
      SPEED_DATA_CASE(5);
      SPEED_DATA_CASE(6);
      SPEED_DATA_CASE(7);
      SPEED_DATA_CASE(8);

    default:
      goto on_error;
      break;
    }

  if (s & CSTOPB)
    c->stop = 1;

  if (s & PARENB)
    {
      if (s & PARODD)
	c->parity |= SERIAL_PARITY_ODD;
      else
	c->parity |= SERIAL_PARITY_EVEN;	
    }

  return 0;

 on_error:

  return -1;
}



/* exported
 */


int serial_open(serial_handle_t* h, const char* path)
{
  invalid_handle(h);

  if ((h->fd = open(path, O_RDWR | O_NONBLOCK | O_NOCTTY)) == -1)
    {
      DEBUG_ERROR("open() == %u\n", errno);
      return -1;
    }

  if (tcgetattr(h->fd, &h->termios) == -1)
    {
      DEBUG_ERROR("tcgetattr() == %u\n", errno);
      goto on_error;
    }

#if CONFIG_SERIAL_DEBUG

  h->name = strdup(path);

  if (h->name == NULL)
    DEBUG_ERROR("h->name == NULL\n");

#endif

  /* success
   */

  return 0;

 on_error:

  close(h->fd);

  invalid_handle(h);

  return -1;
}


void serial_close(serial_handle_t* h)
{
  /* assume h->fd valid
   */

#if CONFIG_SERIAL_DEBUG
  if (h->name != NULL)
    free(h->name);
#endif

  tcsetattr(h->fd, TCSANOW, &h->termios);

  close(h->fd);

  invalid_handle(h);
}


int serial_flush_txrx(serial_handle_t* serial)
{
  if (tcflush(serial->fd, TCIOFLUSH))
  {
    perror("tcflush");
    return -1;
  }

  return 0;
}


int serial_set_conf(serial_handle_t* h, const serial_conf_t* c)
{
  struct termios termios;

#if 1
  memset(&termios, 0, sizeof(struct termios));
  cfmakeraw(&termios);
  if (!(termios.c_cflag = conf_to_speed_t(c)))
    {
      DEBUG_ERROR("conf_to_speed_t()\n");
      goto on_error;
    }
  /* disable hardware flow control */
  termios.c_cflag &= ~CRTSCTS;
#else
  tcgetattr(h->fd, &termios);
  cfsetospeed(&termios, (speed_t)B38400);
  cfsetispeed(&termios, (speed_t)B38400);
  termios.c_cflag = (termios.c_cflag & ~CSIZE) | CS8;
  termios.c_iflag = IGNBRK;
  termios.c_lflag = 0;
  termios.c_oflag = 0;
  termios.c_cflag |= CLOCAL | CREAD;
  termios.c_iflag &= ~(IXON | IXOFF | IXANY);
  termios.c_cflag &= ~(PARENB | PARODD);
  termios.c_cflag &= ~CSTOPB;
#endif

  if (tcsetattr(h->fd, TCSANOW, &termios) == -1)
    {
      DEBUG_ERROR("tcsetattr() == %u\n", errno);
      goto on_error;
    }

  return 0;

 on_error:

  return -1;
}


int serial_get_conf(serial_handle_t* h,
		    serial_conf_t* c)
{
  struct termios termios;

  memset(c, 0, sizeof(serial_conf_t));

  memset(&termios, 0, sizeof(struct termios));

  if (tcgetattr(h->fd, &termios) == -1)
    {
      DEBUG_ERROR("tcgetattrs() == %u\n", errno);
      goto on_error;
    }

  if (speed_t_to_conf(termios.c_ospeed, c) == -1)
    {
      DEBUG_ERROR("speed_t_to_conf()\n");
      goto on_error;
    }

  return 0;

 on_error:

  return -1;
}


int serial_read(serial_handle_t* h, void* buf, size_t size, size_t* nread)
{
  ssize_t n;

  *nread = 0;

  n = read(h->fd, buf, size);

  if (n == -1)
  {
    DEBUG_ERROR("read() == %u\n", errno);
    return -1;
  }

  *nread = n;

  return 0;
}


int serial_readn(serial_handle_t* h, void* buf, size_t size)
{
  ssize_t n;

  while (size)
  {
    n = read(h->fd, buf, size);
    if (n < 0)
    {
      perror("read()\n");
      return -1;
    }
    else if (n)
    {
      size -= n;
      buf = (unsigned char*)buf + n;
    }
  }

  return 0;
}


int serial_writen(serial_handle_t* h, const void* buf, size_t size)
{
  ssize_t n;

  while (size)
  {
    n = write(h->fd, buf, size);
    if (n < 0)
    {
      perror("write()\n");
      return -1;
    }
    else if (n)
    {
      if (tcdrain(h->fd))
      {
	DEBUG_ERROR();
	return -1;
      }

      size -= n;
      buf = (unsigned char*)buf + n;
    }
  }

  return 0;
}


int serial_write
(serial_handle_t* h, const void* buf, size_t size, size_t* nwritten)
{
  ssize_t n;

  *nwritten = 0;

  n = write(h->fd, buf, size);
  if (n == -1)
  {
    DEBUG_ERROR("write() == %u\n", errno);
    return -1;
  }

  if (tcdrain(h->fd))
  {
    DEBUG_ERROR();
    return -1;
  }

  *nwritten = n;

  return 0;
}


#if  CONFIG_SERIAL_DEBUG

void serial_print(serial_handle_t* h)
{
  serial_conf_t c;

  printf("serial[%s]\n", h->name);

  if (serial_get_conf(h, &c) == -1)
    {
      printf("[!] serial_get_conf()\n");
      return ;
    }

  printf("{\n");
  printf(" .bauds: %u\n", c.bauds);
  printf(" .data: %u\n", c.data);
  printf(" .stop: %u\n", c.stop);
  printf(" .parity: %u\n", c.parity);
  printf("}\n");
}

#endif /* CONFIG_SERIAL_DEBUG */
