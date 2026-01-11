#include <iostream>

#include "include/testmain.h"
#include "include/sample_pcm_s16le.c"
#include "include/hashvalue.c"

#include <noson/audioformat.h>
#include <noson/flacencoder.h>

class OutputBuffer : public SONOS::OutputStream
{
  std::vector<char> m_buffer;
public:
  OutputBuffer() { }
  ~OutputBuffer() { }
  int Write(const char* data, int len) override
  {
    m_buffer.insert(m_buffer.end(), data, data + len);
    return len;;
  }
  size_t size() { return m_buffer.size(); }
};

#define BUFSIZE 1024

TEST_CASE("Encoding PCM s16le to FLAC")
{
  SONOS::FLACEncoder encoder;

  int retry = 5;
  while (0 < retry--) {
    OutputBuffer output;
    REQUIRE(encoder.open(SONOS::AudioFormat::CDLPCM(), &output) == true);

    unsigned char * p = pcm_s16le_raw;
    unsigned char * e = pcm_s16le_raw + pcm_s16le_raw_len;

    while (p < e)
    {
      size_t s = BUFSIZE;
      if ((p + s) > e)
        s = e - p;
      // push the data to the encoder
      int r = encoder.Write((char*)p, s);
      if (r <= 0)
        break;
      p += r;
    }

    REQUIRE(p == e);
    encoder.close();

    std::cout << "Encoding ratio: " << output.size() << "/" << pcm_s16le_raw_len << std::endl;
    REQUIRE(output.size() > (pcm_s16le_raw_len / 2));
  }
}
