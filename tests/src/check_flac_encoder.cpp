#include <iostream>

#include "include/testmain.h"
#include "include/sample_pcm_s16le.c"
#include "include/hashvalue.c"

#include <noson/audioformat.h>
#include <noson/flacencoder.h>

// buffer for data
char buf[1024];

uint64_t _flush_encoded_data(SONOS::AudioEncoder * encoder)
{
  uint64_t h = 0;
  int w = 0;
  // check for available encoded data and flush out
  while ((w = encoder->bytesAvailable()) > 0)
  {
    if ((w = encoder->read(buf, sizeof(buf), 0/*forever*/)) > 0)
      h += hashvalue(0xffffffff, buf, w);
  }
  return h;
}

TEST_CASE("Encoding PCM s16le to FLAC")
{
  SONOS::FLACEncoder * encoder = new SONOS::FLACEncoder(1024);
  encoder->setAudioFormat(SONOS::AudioFormat::CDLPCM());

  int retry = 5;
  while (0 < retry--) {
    uint64_t hash = 0;

    REQUIRE(encoder->open() == true);
    REQUIRE(encoder->open() == false);

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
      // flush out
      hash += _flush_encoded_data(encoder);
    }

    REQUIRE(p == e);
    encoder->close();

    // check the buffer is cleaned on reopening
    if ((retry & 1) == 1)
      continue;

    // flush out the rest of encoded data
    hash += _flush_encoded_data(encoder);
    REQUIRE(hash == 0xa3c1d6c5);
  }

  delete encoder;
}
