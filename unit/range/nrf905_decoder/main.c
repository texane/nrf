/* sudo rtl_fm -f 433000000 -s 1600k -g 0 | ./a.out - */

/* center frequency: 433MHz */
/* frequency deviation: 100KHz */
/* data rate: 100Kbps (symbol rate 50Kbps) */


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>


/* nrf905 decoder */

#define NRF905_FLAG_NONE 0
#define NRF905_FLAG_IS_CRC8 (1 << 0)
#define NRF905_FLAG_IS_CRC16 (1 << 1)
#define NRF905_FLAG_IS_THRESHOLD (1 << 2)

typedef struct
{
  size_t pwidth;
  size_t awidth;

  unsigned int flags;

  /* manchester bits */
  uint8_t manch_bits[2];
  size_t manch_pos;

  /* manchester frame size */
  size_t frame_size;

  int16_t* buf;
  size_t pos;
  size_t size;

  /* downsampling */
  size_t ds_count;
  size_t ds_pos;
  int32_t ds_sum;

  int16_t lo_thr;
  int16_t hi_thr;
  int32_t sum_thr;
  size_t pos_thr;

} nrf905_handle_t;

static void nrf905_init
(
 nrf905_handle_t* nrf,
 size_t pwidth,
 size_t awidth,
 unsigned int flags
)
{
  /* pwidth the payload width, in bytes */
  /* pwidth the address width, in bytes */
  /* size the buffer size */

  size_t cwidth;

  nrf->pwidth = pwidth;
  nrf->awidth = awidth;
  nrf->flags = flags;
  nrf->pos = 0;

  nrf->manch_pos = 0;

  nrf->ds_count = 16;
  nrf->ds_pos = 0;
  nrf->ds_sum = 0;

  nrf->sum_thr = 0;
  nrf->pos_thr = 0;
  nrf->lo_thr = 0;
  nrf->hi_thr = 0;

  /* in manchester coded bits */
  if (flags & NRF905_FLAG_IS_CRC8) cwidth = 1;
  else if (flags & NRF905_FLAG_IS_CRC16) cwidth = 2;
  else cwidth = 0;
  nrf->frame_size = 10 + (pwidth + awidth + cwidth) * 8;

  nrf->buf = malloc(nrf->frame_size * sizeof(int16_t));
  nrf->size = nrf->frame_size;
}

static void nrf905_fini(nrf905_handle_t* nrf)
{
  free(nrf->buf);
}

static void nrf905_decode
(nrf905_handle_t* nrf, const uint8_t* buf, size_t size)
{
  /* buffer format: sle16 */
  /* gfsk filter, manchester encoding */

  const size_t n = size / sizeof(int16_t);
  size_t i;

  for (i = 0; i != n; ++i)
  {
    int16_t x = *(((const int16_t*)buf) + i);

    if ((nrf->flags & NRF905_FLAG_IS_THRESHOLD) == 0)
    {
      nrf->sum_thr += x;
      nrf->pos_thr += 1;

      if (nrf->pos_thr != 256) continue ;

      nrf->sum_thr = nrf->sum_thr / nrf->pos_thr;

      nrf->hi_thr = nrf->sum_thr + 64;
      nrf->lo_thr = nrf->sum_thr - 64;

      nrf->flags |= NRF905_FLAG_IS_THRESHOLD;
    }

    nrf->ds_sum += x;

    /* downsampling */
    if ((++nrf->ds_pos) == nrf->ds_count)
    {
      x = nrf->ds_sum / (int32_t)nrf->ds_count;

      nrf->ds_sum = 0;
      nrf->ds_pos = 0;

      if (x > nrf->hi_thr)
      {
	if (x > (nrf->hi_thr + 1500)) goto do_skip;
      }
      else if (x < nrf->lo_thr)
      {
	if (x < (nrf->lo_thr - 1500)) goto do_skip;
      }
      else
      {
      do_skip:
      	nrf->manch_pos = 0;
      	continue ;
      }

      if (x < nrf->lo_thr) nrf->manch_bits[nrf->manch_pos] = 0;
      else nrf->manch_bits[nrf->manch_pos] = 1;
      ++nrf->manch_pos;
    }

    /* manchester clock is twice the sampling clock */
    if (nrf->manch_pos == 2)
    {
      if (nrf->manch_bits[0] == nrf->manch_bits[1])
      {
	/* should not occur */
	nrf->pos = 0;
	nrf->manch_pos = 0;
	continue ;
      }

      if ((nrf->manch_bits[0] == 1) && (nrf->manch_bits[1] == 0))
	nrf->buf[nrf->pos] = 0;
      else if ((nrf->manch_bits[0] == 0) && (nrf->manch_bits[1] == 1))
	nrf->buf[nrf->pos] = 1;

      nrf->pos += 1;
      nrf->manch_pos = 0;
    }

    if (nrf->pos == nrf->frame_size)
    {
      /* decode frame from nrf->buf bitstream */

      size_t j;
      size_t k;

      for (j = 0; j < nrf->frame_size; j += 8)
      {
	uint8_t x = 0;
	for (k = 0; k != 8; ++k)
	  x |= ((nrf->buf[2 + j + k] ? 1 : 0) << (7 - k));
	printf("%02x", x);
      }
      printf("\n");

      nrf->pos = 0;
    }
  }
}

/* main */

int main(int ac, char** av)
{
  int fd;
  size_t size;
  nrf905_handle_t nrf;
  uint8_t buf[512];

  if (av[1][0] != '-')
  {
    fd = open(av[1], O_RDONLY);
    if (fd == -1) return -1;
  }
  else
  {
    fd = 0;
  }

  nrf905_init(&nrf, 8, 4, NRF905_FLAG_NONE);

  while (1)
  {
    size = read(fd, buf, sizeof(buf));
    if (size <= 0) break ;
    nrf905_decode(&nrf, buf, size);
  }

  nrf905_fini(&nrf);

  if (fd != 0) close(fd);

  return 0;
}
