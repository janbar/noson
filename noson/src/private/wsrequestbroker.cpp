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
#include "uriencoder.h"
#include "uriparser.h"
#include "socket.h"
#include "debug.h"
#include "builtin.h"
#include "tokenizer.h"

#define HTTP_TOKEN_MAXLEN     80
#define HTTP_HEADER_MAXLEN    4000
#define QUERY_BUFFER_SIZE     0x1000
#define CHUNK_MAX_SIZE        0x20000

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
  while (l < HTTP_HEADER_MAXLEN);

  *len = l;
  return true;
}

bool WSRequestBroker::ExplodeURI(const std::string& uri, std::string& path, std::string& uriparams, bool& ishidden)
{
  if (uri.empty())
    return false;
  // convert generic uri * to nil
  else if (uri == "*")
  {
    path.clear();
    uriparams.clear();
    ishidden = false;
    return true;
  }
  // accept only local uri, therefore reject url starting by scheme
  else if (uri.front() != '/')
    return false;

  // force sheme and parse uri
  URIParser parser(std::string("file://").append(uri));
  if (!parser.Path())
    return false;
  unsigned len = 0;
  bool hidden = false;
  std::vector<std::string> dirty;
  std::vector<std::string> clean;
  std::vector<std::string>::const_iterator it;
  // decode and split path by dirname
  std::string r_path(pathdecode(parser.Path()));
  tokenize(r_path, "/", "", dirty, true);
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
  for (auto& str : clean)
    path.append("/").append(str);
  if (path.empty() || r_path.back() == '/')
    path.push_back('/');
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

bool WSRequestBroker::ExplodeHost(const std::string& host, std::string& nameStr, std::string& portStr)
{
  std::vector<std::string> tokens;
  if (!tokenize(host, ",", "\"", tokens) || tokens.empty() || tokens[0].empty())
    return false;
  std::string& val = tokens[0];
  if (val.front() == '[')
  {
    size_t n = val.find(']');
    if (n == std::string::npos)
      return false;
    nameStr.assign(val, 0, ++n);
    if (val.size() > (n+1) && val[n] == ':')
      portStr = val.substr(n+1);
  }
  else
  {
    size_t n = val.find(':');
    nameStr.assign(val, 0, n);
    if (n != std::string::npos && val.size() > ++n)
      portStr.assign(val.substr(n));
  }
  return true;
}

WSRequestBroker::WSRequestBroker(TcpSocket* socket, bool secure, int timeout)
: m_socket(socket)
, m_secure(secure)
, m_parsed(false)
, m_method(WS_METHOD_UNKNOWN)
, m_rewritten(false)
, m_pathIsHidden(false)
, m_hasContent(false)
, m_contentChunked(false)
, m_chunkNext(false)
, m_contentLength(0)
, m_consumed(0)
, m_chunkBuffer(nullptr)
, m_chunkPtr(nullptr)
, m_chunkEOR(nullptr)
, m_chunkEnd(nullptr)
, m_status(WS_STATUS_UNKNOWN)
, m_bytesOut(0)
{
  SetTimeout(timeout);
  m_parsed = ParseQuery();
}

WSRequestBroker::~WSRequestBroker()
{
  if (m_chunkBuffer)
    delete [] m_chunkBuffer;
  m_chunkBuffer = m_chunkPtr = m_chunkEOR = m_chunkEnd = nullptr;
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

const std::string& WSRequestBroker::GetRequestHeader(const std::string& key) const
{
  static std::string emptyStr = "";
  VARS::const_iterator it = m_requestHeaders.find(key);
  if (it != m_requestHeaders.end())
    return it->second.Last();
  return emptyStr;
}

bool WSRequestBroker::ParseQuery()
{
  size_t len;
  std::string strread;
  char token[HTTP_TOKEN_MAXLEN + 1];
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
      // backup initial request for logging
      m_requestLine.assign(strread);
      std::vector<std::string> query;
      tokenize(strread, " ", "", query, true);
      if (query.size() == 3)
      {
        // check the requested method
        m_methodKey = query[0];
        m_method = ws_method_from_str(query[0].c_str());
        // explode requested uri
        if (!ExplodeURI(query[1], m_path, m_uriParams, m_pathIsHidden))
          return false;
        m_requestUri = query[1];
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
      if ((token_len = val - line) > HTTP_TOKEN_MAXLEN)
        token_len = HTTP_TOKEN_MAXLEN;
      for (p = 0; p < token_len; ++p)
        token[p] = toupper(line[p]);
      token[token_len] = 0;
      value_len = len - (val - line + 1);
      while (value_len > 0 && (*(++val) == ' ' || *val == '\t')) --value_len;
      /*
       * An existing field will be amended by appending the content.
       * Therefore, the order of the values remains the same.
       */
      WSHeader& hv = m_requestHeaders[token];
      hv.SetName(line, token_len);
      hv.MergeValue("");
    }
    else
    {
      /* Unknown syntax! Close previous token */
      token_len = 0;
      token[token_len] = 0;
    }

    if (token_len && val)
    {
      /* append the content */
      std::string& newval = m_requestHeaders[token].Back().append(val);
      switch (ws_header_from_upperstr(token))
      {
      case WS_HEADER_Content_Length:
      {
        int64_t num;
        if (string_to_int64(newval.c_str(), &num) != 0 || num < 0)
          ret = false;
        else if (num > 0)
        {
          m_hasContent = true;
          m_contentLength = (size_t)num;
        }
        break;
      }
      case WS_HEADER_Transfer_Encoding:
        if (newval.find("chunked") != std::string::npos)
        {
          m_hasContent = true;
          m_contentChunked = true;
          m_chunkNext = true;
        }
        break;
      case WS_HEADER_Host:
        if (!ExplodeHost(newval, m_serverName, m_serverPort))
          ret = false;
        else
          m_host.assign(newval);
        break;
      default:
        break;
      }
    }
  }

  // the server name is required
  return (ret && !m_serverName.empty());
}

int WSRequestBroker::ReadChunk(void *buf, size_t buflen)
{
  int s = 0;
  if (m_contentChunked)
  {
    // no more pending byte in chunk buffer
    if (m_chunkPtr >= m_chunkEnd)
    {
      // process next chunk
      if (m_chunkBuffer)
        delete [] m_chunkBuffer;
      m_chunkBuffer = m_chunkPtr = m_chunkEOR = m_chunkEnd = nullptr;
      std::string strread;
      size_t len = 0;
      while (ReadHeaderLine(WS_CRLF, strread, &len) && len == 0);
      DBG(DBG_PROTO, "%s: chunked data (%s)\n", __FUNCTION__, strread.c_str());
      std::string chunkStr("0x0");
      uint32_t chunkSize;
      if (strread.empty() || sscanf(chunkStr.append(strread.substr(0, strread.find(','))).c_str(), "%x", &chunkSize) != 1)
        return (-1);
      if (chunkSize > 0)
      {
        // check chunk-size overflow
        if (chunkSize > CHUNK_MAX_SIZE)
        {
          DBG(DBG_ERROR, "%s: chunk-size overflow (req=%u) (max=%u)\n", __FUNCTION__, chunkSize, (unsigned)CHUNK_MAX_SIZE);
          return (-1);
        }
        if (!(m_chunkBuffer = new char[chunkSize]))
          return (-1);
        m_chunkPtr = m_chunkEOR = m_chunkBuffer;
        m_chunkEnd = m_chunkBuffer + chunkSize;
      }
      else
      {
        // read chunk trailers
        while (ReadHeaderLine(WS_CRLF, strread, &len) && len != 0);
        return 0; // that's the end of chunks
      }
    }
    // fill chunk buffer
    if (m_chunkPtr >= m_chunkEOR)
    {
      // ask for new data to fill in the chunk buffer
      // fill at last read position and until to the end
      m_chunkEOR += m_socket->ReceiveData(m_chunkEOR, m_chunkEnd - m_chunkEOR);
    }
    if ((s = m_chunkEOR - m_chunkPtr) < 0)
      return (-1);
    if (buflen < (size_t)s)
      s = (int)buflen;
    memcpy(buf, m_chunkPtr, s);
    m_chunkPtr += s;
    m_consumed += s;
  }
  return s;
}

int WSRequestBroker::ReadContent(char* buf, size_t buflen)
{
  if (!m_contentChunked)
  {
    if (m_contentLength > m_consumed)
    {
      size_t len = m_contentLength - m_consumed;
      int s = (int)m_socket->ReceiveData(buf, len > buflen ? buflen : len);
      if (s <= 0)
        m_consumed = m_contentLength;
      else
        m_consumed += s;
      return s;
    }
    return 0;
  }
  else if (m_chunkNext)
  {
    int s = ReadChunk(buf, buflen);
    if (s <= 0)
      m_chunkNext = false;
    return s;
  }
  return 0;
}

bool WSRequestBroker::ReplyData(const char* data, size_t size)
{
  m_bytesOut += size;
  return m_socket->SendData(data, size);
}

bool WSRequestBroker::RewritePath(const std::string& newpath)
{
  std::string path;
  std::string params;
  bool hidden;
  if (ExplodeURI(pathencode(newpath), path, params, hidden))
  {
    // do not override query params
    m_path = path;
    m_pathIsHidden = hidden;
    m_rewritten = true;
    return true;
  }
  return false;
}
