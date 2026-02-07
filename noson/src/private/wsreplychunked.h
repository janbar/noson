
#ifndef WSREPLYCHUNKED_H
#define WSREPLYCHUNKED_H

#include "local_config.h"
#include "wsrequestbroker.h"
#include "iostream.h"
#include "wsstatic.h"

#include <string>

namespace NSROOT
{

class WSReplyChunked : public OutputStream
{
public:
  WSReplyChunked(WSRequestBroker& rb, int ckunkSize);
  virtual ~WSReplyChunked();

  virtual int Write(const char * data, int len) override;

  bool Flush();
  int ReadFileStream(FILE * file);
  int ReadFileStream(FILE * file, unsigned maxlen);
  int ReadInputStream(InputStream& in);

private:
  WSRequestBroker& m_broker;
  int m_chunkSize;
  char * m_buffer;
  int m_head;
  int m_tail;

  bool WriteChunk(const char * data, int len);
};

}

#endif /* WSREPLYCHUNKED_H */

