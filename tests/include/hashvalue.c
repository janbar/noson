#include <stdint.h>

static uint_fast32_t hashvalue(uint_fast32_t maxsize, const char * buf, size_t len)
{
  uint_fast32_t h = 0, g;
  const char * end = buf + len;

  while (buf < end)
  {
    h = (h << 4) + *buf++;
    if ((g = h & 0xF0000000L))
    {
      h ^= g >> 24;
    }
    h &= ~g;
  }

  return h % maxsize;
}
