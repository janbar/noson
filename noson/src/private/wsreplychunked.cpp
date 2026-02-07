
#include "wsreplychunked.h"
#include "wsrequestbroker.h"
#include "debug.h"

#define CHUNK_MAXSIZE   0x10000

using namespace NSROOT;

WSReplyChunked::WSReplyChunked(WSRequestBroker& broker, int chunkSize)
: m_broker(broker)
, m_chunkSize(CHUNK_MAXSIZE)
{
  if (chunkSize > 0 && chunkSize <= CHUNK_MAXSIZE)
    m_chunkSize = chunkSize;

  m_buffer = new char [m_chunkSize];
  m_head = 0;
  m_tail = m_chunkSize;
}

WSReplyChunked::~WSReplyChunked()
{
  delete [] m_buffer;
}

int WSReplyChunked::Write(const char * data, int len)
{
  int rest = len;
  while (rest)
  {
    if (rest <= m_tail)
    {
      memcpy(m_buffer + m_head, data, rest);
      m_head += rest;
      m_tail -= rest;
      rest = 0;
    }
    else
    {
      if (m_head)
      {
        if (m_tail)
        {
          memcpy(m_buffer + m_head, data, m_tail);
          data += m_tail;
          rest -= m_tail;
        }
        if (!WriteChunk(m_buffer, m_chunkSize))
          return 0;
      }
      while (rest >= m_chunkSize)
      {
        if (!WriteChunk(data, m_chunkSize))
          return 0;
        data += m_chunkSize;
        rest -= m_chunkSize;
      }
      m_head = 0;
      m_tail = m_chunkSize;
    }
  }
  return len;
}

bool WSReplyChunked::Flush()
{
  if (m_head)
  {
    if (!WriteChunk(m_buffer, m_head))
      return false;
  }
  if (m_broker.ReplyData("0" WS_CRLF WS_CRLF, sizeof("0") + WS_CRLF_LEN + WS_CRLF_LEN))
    return true;
  DBG(DBG_WARN, "%s: chunk %p failed\n", __FUNCTION__, &m_broker);
  return false;
}

int WSReplyChunked::ReadFileStream(FILE * file)
{
  if (!m_tail)
  {
    if (!WriteChunk(m_buffer, m_chunkSize))
      return (-1);
    m_head = 0;
    m_tail = m_chunkSize;
  }

  int r = 0;
  if ((r = (int)fread(m_buffer + m_head, 1, m_tail, file)) > 0)
  {
    m_head += r;
    m_tail -= r;
  }
  return r;
}

int WSReplyChunked::ReadFileStream(FILE * file, unsigned maxlen)
{
  if (!m_tail)
  {
    if (!WriteChunk(m_buffer, m_chunkSize))
      return (-1);
    m_head = 0;
    m_tail = m_chunkSize;
  }

  if (maxlen > (unsigned)m_tail)
    maxlen = m_tail;
  int r = 0;
  if ((r = (int)fread(m_buffer + m_head, 1, maxlen, file)) > 0)
  {
    m_head += r;
    m_tail -= r;
  }
  return r;
}

int WSReplyChunked::ReadInputStream(InputStream& in)
{
  if (!m_tail)
  {
    if (!WriteChunk(m_buffer, m_chunkSize))
      return (-1);
    m_head = 0;
    m_tail = m_chunkSize;
  }

  int r = 0;
  if ((r = in.Read(m_buffer + m_head, m_tail)) > 0)
  {
    m_head += r;
    m_tail -= r;
  }
  return r;
}

bool WSReplyChunked::WriteChunk(const char * data, int len)
{
  DBG(DBG_PROTO, "%s: chunk %p (%u)\n", __FUNCTION__, &m_broker, (unsigned)len);
  char str[6 + WS_CRLF_LEN];
  snprintf(str, sizeof(str), "%05x" WS_CRLF, (unsigned)len & 0xfffff);
  if (m_broker.ReplyData(str, 5 + WS_CRLF_LEN) &&
          m_broker.ReplyData(data, len) &&
          m_broker.ReplyData(WS_CRLF, WS_CRLF_LEN))
    return true;
  DBG(DBG_WARN, "%s: chunk %p failed\n", __FUNCTION__, &m_broker);
  return false;
}