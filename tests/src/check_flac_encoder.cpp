#include <iostream>

#include "include/testmain.h"
#include "include/sample_pcm_s16le.c"
#include "include/hashvalue.c"

#include <noson/audioformat.h>
#include <noson/flacencoder.h>

uint64_t _flush_encoded_data(SONOS::AudioEncoder * encoder, char * buf, size_t bufsize)
{
  uint64_t h = 0;
  int w = 0;
  // check for available encoded data and flush out
  while ((w = encoder->bytesAvailable()) > 0)
  {
    if ((w = encoder->read(buf, bufsize, 0/*forever*/)) > 0)
    {
      h += w;
      buf += w;
    }
  }
  return h;
}

#define BUFSIZE 1024

TEST_CASE("Encoding PCM s16le to FLAC")
{
  char * buf = new char[BUFSIZE];
  char * flc = new char[pcm_s16le_raw_len];
  uint64_t flc_sz = 0;
  SONOS::FLACEncoder * encoder = new SONOS::FLACEncoder(256);
  encoder->setAudioFormat(SONOS::AudioFormat::CDLPCM());

  int retry = 5;
  while (0 < retry--) {
    uint64_t hash = 0;

    REQUIRE(encoder->open() == true);
    REQUIRE(encoder->open() == false);

    unsigned char * p = pcm_s16le_raw;
    unsigned char * e = pcm_s16le_raw + pcm_s16le_raw_len;

    char * p_flc = flc;
    while (p < e)
    {
      size_t s = BUFSIZE;
      if ((p + s) > e)
        s = e - p;
      // push the data to the encoder
      int r = encoder->write((char*)p, s);
      if (r <= 0)
        break;
      p += r;
      // flush out
      p_flc += _flush_encoded_data(encoder, p_flc, BUFSIZE);
    }

    REQUIRE(p == e);
    encoder->close();

    // check the buffer is cleaned on reopening
    if ((retry & 1) == 1)
      continue;

    // flush out the rest of encoded data
    p_flc += _flush_encoded_data(encoder, p_flc, BUFSIZE);
    flc_sz = p_flc - flc;
    std::cout << "Encoding ratio: " << flc_sz << "/" << pcm_s16le_raw_len << std::endl;
    REQUIRE(flc_sz > (pcm_s16le_raw_len / 2));
  }


  delete encoder;
  delete [] flc;
  delete [] buf;
}
