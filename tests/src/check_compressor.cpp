#include <iostream>

#include "include/testmain.h"
#include "include/sample_pcm_s16le.c"
#include "include/hashvalue.c"

#include <private/compressor.h>

size_t _flush_compressed_data(SONOS::Compressor * c, char * out)
{
  size_t l = 0;
  int w = 0;
  // check for available encoded data and flush out
  while ((w = c->ReadOutput(out, 1024)) > 0)
  {
    out += w;
    l += w;
  }
  return l;
}

size_t _flush_decompressed_data(SONOS::Decompressor * d, char * out)
{
  size_t l = 0;
  int w = 0;
  // check for available encoded data and flush out
  while ((w = d->ReadOutput(out, 1024)) > 0)
  {
    out += w;
    l += w;
  }
  return l;
}

TEST_CASE("Compress Z data")
{
  SONOS::Compressor * c =  new SONOS::Compressor((const char*)pcm_s16le_raw, pcm_s16le_raw_len);
  char * zip = new char[pcm_s16le_raw_len];
  size_t lz = _flush_compressed_data(c, zip);
  REQUIRE(c->HasStreamError() == false);
  REQUIRE(c->HasBufferError() == false);
  REQUIRE(c->IsCompleted() == true);
  REQUIRE((lz > 0 && lz < pcm_s16le_raw_len));

  char * raw = new char[pcm_s16le_raw_len];
  SONOS::Decompressor * d =  new SONOS::Decompressor((const char*)zip, lz);
  size_t lr = _flush_decompressed_data(d, raw);
  REQUIRE(d->HasStreamError() == false);
  REQUIRE(d->HasBufferError() == false);
  REQUIRE(d->IsCompleted() == true);
  REQUIRE(lr == pcm_s16le_raw_len);
  REQUIRE((hashvalue(0xffffffff, raw, lr) == hashvalue(0xffffffff, (const char *)pcm_s16le_raw, pcm_s16le_raw_len)));

  delete d;
  delete c;
  delete [] zip;
  delete [] raw;
}

