#include <iostream>

#include "include/testmain.h"
#include "include/sample_pcm_s16le.c"
#include "include/hashvalue.c"

#include <noson/audioformat.h>
#include <noson/flacencoder.h>

TEST_CASE("Encoding PCM s16le to FLAC")
{
  SONOS::FLACEncoder * encoder = new SONOS::FLACEncoder(1024);
  encoder->setAudioFormat(SONOS::AudioFormat::CDLPCM());
  encoder->open();

  uint64_t hash = 0;

  // buffer for data
  char buf[1024];
  // pcm stream
  unsigned char * p = pcm_s16le_raw;
  unsigned char * e = pcm_s16le_raw + pcm_s16le_raw_len;

  while (p < e)
  {
    size_t s = sizeof(buf);
    if ((p + s) > e)
      s = e - p;
    // push the data to the encoder
    int r = encoder->write((char*)p, s);
    if (r <= 0)
      break;
    p += r;
    // check for available encoded data and flush out
    int w = 0;
    while ((w = encoder->bytesAvailable()) > 0)
    {
      if ((w = encoder->read(buf, sizeof(buf), 0/*forever*/)) > 0)
      {
        uint32_t h = hashvalue(0xffffffff, buf, w);
        hash += h;
      }
    }
  }

  encoder->close();

  // flush out the rest of encoded data
  int w = 0;
  while ((w = encoder->bytesAvailable()) > 0)
  {
    if ((w = encoder->read(buf, sizeof(buf), 1/*forever*/)) > 0)
    {
        uint32_t h = hashvalue(0xffffffff, buf, w);
        hash += h;
    }
  }

  delete encoder;

  REQUIRE(hash == 0xa3c1d6c5);
}
