
#ifndef WSREPLYCHUNKED_H
#define WSREPLYCHUNKED_H

#include "local_config.h"
#include "wsrequestbroker.h"
#include "iostream.h"

namespace NSROOT
{

class WSReplyChunked : public OutputStream
{
public:
  WSReplyChunked(WSRequestBroker& rb, int ckunkSize);
  virtual ~WSReplyChunked();

  /**
   * Write data into the chunked stream.
   * @param data
   * @param len The size of data
   * @return 0 on failure, else the size of data
   */
  virtual int Write(const char * data, int len) override;

  /**
   * Finalize the chunked stream
   * @return true on success, else false
   */
  bool Flush();

  /**
   * Write FILE stream into the chunked stream.
   * @param file The file stream to be read
   * @return The number of bytes actually read
   */
  int WriteFileStream(FILE * file);

  /**
   * Write FILE stream into the chunked stream.
   * @param file The file stream to be read
   * @param maxlen The maximum number of bytes to read
   * @return The number of bytes actually read
   */
  int WriteFileStream(FILE * file, unsigned maxlen);


  /**
   * Write input stream into the chunked stream.
   * @param in The input stream to be read
   * @return The number of bytes actually read
   */
  int WriteInputStream(InputStream& in);

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

