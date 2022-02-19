#if (defined(_WIN32) || defined(_WIN64))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__
#include <WinSock2.h>
#include <Windows.h>
#include <time.h>
//#define usleep(t) Sleep((DWORD)(t)/1000)
//#define sleep(t)  Sleep((DWORD)(t)*1000)
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include <cstdio>
#include <string>
#include <cstdlib>
#include <cstring>

#include <noson/sonossystem.h>
#include <noson/audioformat.h>
#include <noson/audiosource.h>
#include <noson/flacencoder.h>

#define PRINT(a) fprintf(stderr, a)
#define PRINT1(a,b) fprintf(stderr, a, b)
#define PRINT2(a,b,c) fprintf(stderr, a, b, c)
#define PRINT3(a,b,c,d) fprintf(stderr, a, b, c, d)
#define PRINT4(a,b,c,d,e) fprintf(stderr, a, b, c, d, e)

void usage(const char* cmd)
{
  fprintf(stderr,
        "Usage: %s [options]\n"
        "  --infile <path>            Path of input file with format PCM S16LE\n"
        "  --outfile <path>           Path of output file (FLAC)\n"
        "  --debug                    Enable debug output\n"
        "  --help                     print this help\n"
        "\n", cmd
        );
}

static int g_loglevel = 2;

/**
 * Implement the file audio source
 */
class Source : private SONOS::AudioSource
{
public:
  Source(const std::string& filepath) : SONOS::AudioSource(1)
  , _filepath(filepath)
  {
    if ((_file = fopen(filepath.c_str(), "rb")))
      SONOS::AudioSource::open(ReadOnly);
    else
      PRINT1("failed to open input file '%s'\n", filepath.c_str());
  }
  virtual ~Source()
  {
    if (_file)
      fclose(_file);
  };

  std::string getName() const override { return _filepath; };
  std::string getDescription() const override { return "file"; };
  SONOS::AudioFormat getFormat() const override { return SONOS::AudioFormat::CDLPCM(); };
  int read(char * data, int maxlen) { return readData(data, maxlen); }

private:
  std::string _filepath;
  FILE * _file              = nullptr;
  int readData(char* data, int maxlen) override
  {
    int r = -1;
    if (_file)
    {
      r = fread(data, 1, maxlen, _file);
      if (r <= 0)
      {
        fclose(_file);
        _file = nullptr;
      }
    }
    return r;
  }
};

/**
 * MAIN
 */
int main(int argc, char** argv)
{
  std::string infilepath;
  std::string outfilepath = "output.flac";

  int i = 0;
  while (++i < argc)
  {
    if (strcmp(argv[i], "--debug") == 0)
    {
      g_loglevel = 4;
      fprintf(stderr, "debug=Yes\n");
    }
    else if (strcmp(argv[i], "--infile") == 0 && argc > i+1)
    {
      fprintf(stderr, "infile=%s\n", argv[i+1]);
      infilepath.assign(argv[i+1]);
    }
    else if (strcmp(argv[i], "--outfile") == 0 && argc > i+1)
    {
      fprintf(stderr, "outfile=%s\n", argv[i+1]);
      outfilepath.assign(argv[i+1]);
    }
    else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
    {
      usage(argv[0]);
      return EXIT_SUCCESS;
    }
  }
  if (infilepath.size() == 0)
  {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  SONOS::System::Debug(g_loglevel);

  FILE * flac = fopen(outfilepath.c_str(), "wb");
  if (flac)
  {
    // new audio source
    Source * source = new Source(infilepath);
    // initialize the encoder and configure the format
    SONOS::FLACEncoder * encoder = new SONOS::FLACEncoder(1024);
    encoder->setAudioFormat(source->getFormat());
    encoder->open();

    // buffer for data
    char buf[1024];

    int r = 0;
    while ((r = source->read(buf, sizeof(buf))) > 0)
    {
      // push the data to the encoder
      encoder->write(buf, r);
      // check for available encoded data and flush out
      int w = 0;
      while ((w = encoder->bytesAvailable()) > 0)
      {
        if ((w = encoder->read(buf, sizeof(buf), 0/*forever*/)) > 0)
          fwrite(buf, 1, w, flac);
      }
    }

    encoder->close();

    // flush out the rest of encoded data
    int w = 0;
    while ((w = encoder->bytesAvailable()) > 0)
    {
      if ((w = encoder->read(buf, sizeof(buf), 1/*forever*/)) > 0)
        fwrite(buf, 1, w, flac);
    }

    fclose(flac);
    delete encoder;
    delete source;
    return EXIT_SUCCESS;
  }

  PRINT1("failed to create output file '%s'\n", outfilepath.c_str());
  return EXIT_FAILURE;
}
