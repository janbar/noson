/*
 *      Copyright (C) 2014-2015 Jean-Luc Barriere
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <filesystem>

#include "wsrequestbroker.h"
#include "urlencoder.h"
#include "uriparser.h"
#include "socket.h"
#include "debug.h"
#include "builtin.h"

#define HTTP_TOKEN_MAXSIZE    20
#define HTTP_HEADER_MAXSIZE   4000
#define QUERY_BUFFER_SIZE     4000
#define CHUNK_MAX_SIZE        0x1FFFF

using namespace NSROOT;

bool WSRequestBroker::ReadHeaderLine(const char *eol, std::string& line, size_t *len)
{
  char buf[QUERY_BUFFER_SIZE];
  const char *s_eol;
  int p = 0, p_eol = 0, l_eol;
  size_t l = 0;

  if (eol != nullptr)
    s_eol = eol;
  else
    s_eol = "\n";
  l_eol = strlen(s_eol);

  line.clear();
  do
  {
    if (m_socket->ReceiveData(&buf[p], 1) > 0)
    {
      if (buf[p++] == s_eol[p_eol])
      {
        if (++p_eol >= l_eol)
        {
          buf[p - l_eol] = '\0';
          line.append(buf);
          l += p - l_eol;
          break;
        }
      }
      else
      {
        p_eol = 0;
        if (p > (QUERY_BUFFER_SIZE - 2 - l_eol))
        {
          buf[p] = '\0';
          line.append(buf);
          l += p;
          p = 0;
        }
      }
    }
    else
    {
      /* No EOL found until end of data */
      *len = l;
      return false;
    }
  }
  while (l < HTTP_HEADER_MAXSIZE);

  *len = l;
  return true;
}

void WSRequestBroker::Tokenize(const std::string& str, char delimiter, std::vector<std::string>& tokens, bool trimnull)
{
  std::string::size_type pa = 0, pb = 0;
  unsigned n = 0;
  // Counter n will break infinite loop. Max count is 255 tokens
  while ((pb = str.find_first_of(delimiter, pb)) != std::string::npos && ++n < 255)
  {
    tokens.push_back(str.substr(pa, pb - pa));
    do
    {
      pa = ++pb;
    } while (trimnull && str.find_first_of(delimiter, pb) == pb);
  }
  tokens.push_back(str.substr(pa));
}

bool WSRequestBroker::NormalizeURI(const std::string& in, std::string& outpath, std::string& outparams)
{
  // parse uri
  URIParser parser(in);
  if (!parser.Path())
    return false;
  unsigned len = 0;
  std::vector<std::string> dirty;
  std::vector<std::string> clean;
  std::vector<std::string>::const_iterator it;
  // decode and split path by dirname
  Tokenize(urldecode(parser.Path()), '/'/*SLASH*/, dirty, true);
  // rebuild normalized path
  it = dirty.cbegin();
  while (it != dirty.cend())
  {
    if (*it == "..")
    {
      // check a path traversal attempt
      if (clean.empty())
        return false;
      clean.pop_back();
    }
    else if (*it != ".")
    {
      clean.push_back(*it);
      len += it->size() + 1;
    }
    ++it;
  }
  // store path string
  outpath.clear();
  outpath.reserve(len);
  it = clean.cbegin();
  while (it != clean.cend())
  {
    outpath.append("/").append(*it);
    ++it;
  }
  // store params string
  if (parser.Params())
    outparams.assign(parser.Params());
  else
    outparams.clear();
  return true;
}

WSRequestBroker::WSRequestBroker(TcpSocket* socket, timeval timeout)
: m_socket(socket)
, m_parsed(false)
, m_parsedMethod(WS_METHOD_Head)
, m_contentChunked(false)
, m_contentLength(0)
, m_consumed(0)
, m_chunkBuffer(nullptr)
, m_chunkPtr(nullptr)
, m_chunkEnd(nullptr)
{
  m_socket->SetTimeout(timeout);
  m_parsed = ParseQuery();
}

WSRequestBroker::~WSRequestBroker()
{
  if (m_chunkBuffer)
    delete [] m_chunkBuffer;
  m_chunkBuffer = m_chunkPtr = m_chunkEnd = nullptr;
}

void WSRequestBroker::SetTimeout(timeval timeout)
{
  m_socket->SetTimeout(timeout);
}

std::string WSRequestBroker::GetHostAddrInfo() const
{
  return m_socket->GetHostAddrInfo();
}

const std::string& WSRequestBroker::GetRequestHeader(const std::string& name)
{
  static std::string emptyStr = "";
  entries_t::const_iterator it = m_namedEntries.find(name);
  if (it != m_namedEntries.end())
    return it->second;
  return emptyStr;
}

bool WSRequestBroker::ParseQuery()
{
  size_t len;
  std::string strread;
  char token[HTTP_TOKEN_MAXSIZE + 1];
  int n = 0, token_len = 0;
  bool ret = false;

  token[0] = 0;
  while (ReadHeaderLine(WS_CRLF, strread, &len))
  {
    const char *line = strread.c_str(), *val = nullptr;
    int value_len = 0;

    DBG(DBG_PROTO, "%s: %s\n", __FUNCTION__, line);
    /*
     * The Request-Line begins with a method token, followed by the
     * Request-URI and the protocol version, and ending with CRLF. The
     * elements are separated by SP characters. No CR or LF are allowed
     * except in the final CRLF sequence
     */
    if (++n == 1)
    {
      std::vector<std::string> query;
      Tokenize(strread, ' '/*SP*/, query, true);
      if (query.size() == 3)
      {
        // check the requested method
        WS_METHOD method = ws_method_from_str(query[0].c_str());
        if (method == WS_METHOD_UNKNOWN)
          return false;
        m_parsedMethod = method;
        // check and normalize the requested uri
        if (!NormalizeURI(query[1], m_parsedURIPath, m_parsedURIParams))
          return false;
        // set the requested protocol
        m_parsedProtocol = query[2];
        // Clear entries for next step
        m_namedEntries.clear();
        ret = true;
      }
    }

    if (len == 0)
    {
      /* End of header */
      break;
    }

    /*
     * Header fields can be extended over multiple lines by preceding each
     * extra line with at least one SP or HT.
     */
    if ((line[0] == ' ' || line[0] == '\t') && token_len)
    {
      /* Append value of previous token */
      val = line;
    }
      /*
       * Each header field consists of a name followed by a colon (":") and the
       * field value. Field names are case-insensitive. The field value MAY be
       * preceded by any amount of LWS, though a single SP is preferred.
       */
    else if ((val = strchr(line, ':')))
    {
      int p;
      if ((token_len = val - line) > HTTP_TOKEN_MAXSIZE)
        token_len = HTTP_TOKEN_MAXSIZE;
      for (p = 0; p < token_len; ++p)
        token[p] = toupper(line[p]);
      token[token_len] = 0;
      while ((value_len = len - (val - line)) && *(++val) == ' ');
    }
    else
    {
      /* Unknown syntax! Close previous token */
      token_len = 0;
      token[token_len] = 0;
    }

    if (token_len && val)
    {
      m_namedEntries[token].append(val);
      switch (ws_header_from_upperstr(token))
      {
      case WS_HEADER_Content_Length:
      {
        uint32_t num;
        if (string_to_uint32(val, &num) == 0)
          m_contentLength = (size_t)num;
        else
          ret = false;
        break;
      }
      default:
        break;
      }
    }
  }

  return ret;
}

size_t WSRequestBroker::ReadChunk(void *buf, size_t buflen)
{
  size_t s = 0;
  if (m_contentChunked)
  {
    if (m_chunkPtr == nullptr || m_chunkPtr >= m_chunkEnd)
    {
      if (m_chunkBuffer)
        delete [] m_chunkBuffer;
      m_chunkBuffer = m_chunkPtr = m_chunkEnd = nullptr;
      std::string strread;
      size_t len = 0;
      while (ReadHeaderLine(WS_CRLF, strread, &len) && len == 0);
      DBG(DBG_PROTO, "%s: chunked data (%s)\n", __FUNCTION__, strread.c_str());
      std::string chunkStr("0x0");
      uint32_t chunkSize = 0;
      if (!strread.empty() && sscanf(chunkStr.append(strread).c_str(), "%x", &chunkSize) == 1 && chunkSize > 0)
      {
        // check chunk-size overflow
        if (chunkSize > CHUNK_MAX_SIZE)
        {
          DBG(DBG_ERROR, "%s: chunk-size overflow (req=%u) (max=%u)\n", __FUNCTION__, chunkSize, (unsigned)CHUNK_MAX_SIZE);
          return 0;
        }
        if (!(m_chunkBuffer = new char[chunkSize]))
          return 0;
        m_chunkPtr = m_chunkBuffer;
        m_chunkEnd = m_chunkBuffer + chunkSize;
        if (m_socket->ReceiveData(m_chunkBuffer, chunkSize) != chunkSize)
          return 0;
      }
      else
        return 0;
    }
    if ((s = m_chunkEnd - m_chunkPtr) > buflen)
      s = buflen;
    memcpy(buf, m_chunkPtr, s);
    m_chunkPtr += s;
    m_consumed += s;
  }
  return s;
}

size_t WSRequestBroker::ReadContent(char* buf, size_t buflen)
{
  size_t s = 0;
  if (!m_contentChunked)
  {
    // let read on unknown length
    if (!m_contentLength)
      s = m_socket->ReceiveData(buf, buflen);
    else if (m_contentLength > m_consumed)
    {
      size_t len = m_contentLength - m_consumed;
      s = m_socket->ReceiveData(buf, len > buflen ? buflen : len);
    }
  }
  else
  {
    s = ReadChunk(buf, buflen);
  }
  m_consumed += s;
  return s;
}
