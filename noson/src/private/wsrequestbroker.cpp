/*
 *      Copyright (C) 2014-2026 Jean-Luc Barriere
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

#include "wsrequestbroker.h"
#include "urlencoder.h"
#include "uriparser.h"
#include "socket.h"
#include "debug.h"
#include "builtin.h"
#include "tokenizer.h"

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

bool WSRequestBroker::ExplodeURI(const std::string& in, std::string& path, std::string& uriparams, bool& ishidden)
{
  // global not allowed
  if (in == "*")
    return false;
  // parse uri
  URIParser parser(std::string("file:///").append(in));
  if (!parser.Path())
    return false;
  unsigned len = 0;
  bool hidden = false;
  std::vector<std::string> dirty;
  std::vector<std::string> clean;
  std::vector<std::string>::const_iterator it;
  // decode and split path by dirname
  tokenize(urldecode(parser.Path()), "/", "", dirty, true);
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
      // check a hidden path traversal attempt
      if (it->front() == '.')
        hidden = true;
    }
    ++it;
  }
  // set hidden path status
  ishidden = hidden;
  // store path string
  path.clear();
  path.reserve(len);
  if (clean.empty())
    path.append("/");
  else
  {
    for (auto& str : clean)
      path.append("/").append(str);
  }
  // store params string
  if (parser.Params())
    uriparams.assign(parser.Params());
  else
    uriparams.clear();
  return true;
}

WSRequestBroker::VARS WSRequestBroker::ExplodeQuery(const std::string& uriparams)
{
  // decode query string
  VARS params;
  std::vector<std::string> tokens;
  tokenize(uriparams, "&", "", tokens, true);
  for (std::string& t : tokens)
  {
    size_t p = t.find_first_of('=');
    if (p != std::string::npos && p < (t.size() - 1))
      params.insert(std::make_pair(urldecode(t.substr(0, p)), urldecode(t.substr(p+1))));
  }
  return params;
}

WSRequestBroker::WSRequestBroker(TcpSocket* socket, bool secure, int timeout)
: m_socket(socket)
, m_secure(secure)
, m_parsed(false)
, m_method(WS_METHOD_UNKNOWN)
, m_pathIsHidden(false)
, m_contentChunked(false)
, m_contentLength(0)
, m_consumed(0)
, m_chunkBuffer(nullptr)
, m_chunkPtr(nullptr)
, m_chunkEnd(nullptr)
{
  SetTimeout(timeout);
  m_parsed = ParseQuery();
}

WSRequestBroker::~WSRequestBroker()
{
  if (m_chunkBuffer)
    delete [] m_chunkBuffer;
  m_chunkBuffer = m_chunkPtr = m_chunkEnd = nullptr;
}

void WSRequestBroker::SetTimeout(int timeout)
{
  struct timeval tv = { timeout, 0 };
  if (timeout == 0)
    tv.tv_usec = 999999;
  m_socket->SetTimeout(tv);
}

std::string WSRequestBroker::GetRemoteAddrInfo() const
{
  return m_socket->GetRemoteAddrInfo();
}

std::string WSRequestBroker::GetHostAddrInfo() const
{
  return m_socket->GetHostAddrInfo();
}

const std::string& WSRequestBroker::GetRequestHeader(const std::string& name) const
{
  static std::string emptyStr = "";
  VARS::const_iterator it = m_requestHeaders.find(name);
  if (it != m_requestHeaders.end())
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
      tokenize(strread, " ", "", query, true);
      if (query.size() == 3)
      {
        // check the requested method
        WS_METHOD method = ws_method_from_str(query[0].c_str());
        if (method == WS_METHOD_UNKNOWN)
          return false;
        m_method = method;
        // explode requested uri
        if (!ExplodeURI(query[1], m_path, m_uriParams, m_pathIsHidden))
          return false;
        // set the requested protocol
        m_protocol = query[2];
        // Clear entries for next step
        m_requestHeaders.clear();
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
      value_len = len - (val - line + 1);
      while (value_len > 0 && (*(++val) == ' ' || *val == '\t')) --value_len;
    }
    else
    {
      /* Unknown syntax! Close previous token */
      token_len = 0;
      token[token_len] = 0;
    }

    if (token_len && val)
    {
      m_requestHeaders[token].append(val);
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

bool WSRequestBroker::ReplyHead(WS_STATUS status)
{
  if (status == WS_STATUS_UNKNOWN)
    status = WS_STATUS_500_Internal_Server_Error;
  std::string data;
  data.reserve(128);
  data.append(REQUEST_PROTOCOL " ").append(ws_status_to_numstr(status)).append(" ").append(ws_status_to_msgstr(status)).append(WS_CRLF);
  data.append(ws_header_to_str(WS_HEADER_Server)).append(": ").append(REQUEST_USER_AGENT).append(WS_CRLF);
  data.append(ws_header_to_str(WS_HEADER_Connection)).append(": close" WS_CRLF);
  return m_socket->SendData(data.c_str(), data.size());
}

bool WSRequestBroker::ReplyBody(const char* data, size_t size) const
{
  return m_socket->SendData(data, size);
}

bool WSRequestBroker::RewritePath(const std::string& newpath)
{
  return ExplodeURI(newpath, m_path, m_uriParams, m_pathIsHidden);
}
