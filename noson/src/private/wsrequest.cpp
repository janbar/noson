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

#include "wsrequest.h"
#include "uriencoder.h"
#include "builtin.h"
#include "debug.h"

#include <cstdio>
#include <cstddef> // for size_t
#include <algorithm>

using namespace NSROOT;

WSRequest::WSRequest(const std::string& server, unsigned port)
: m_server(server)
, m_port(port)
, m_secure_uri(false)
, m_service_url()
, m_service_method(WS_METHOD_Get)
, m_charset(REQUEST_STD_CHARSET)
, m_accept()
, m_contentType(WS_CTYPE_Form)
, m_contentTypeStr()
, m_contentData()
{
  if (port == 443)
    m_secure_uri = true;
  // by default allow content encoding if possible
  RequestAcceptEncoding(true);
}

WSRequest::WSRequest(const std::string& server, unsigned port, bool secureURI)
: m_server(server)
, m_port(port)
, m_secure_uri(secureURI)
, m_service_url()
, m_service_method(WS_METHOD_Get)
, m_charset(REQUEST_STD_CHARSET)
, m_accept()
, m_contentType(WS_CTYPE_Form)
, m_contentTypeStr()
, m_contentData()
{
  // by default allow content encoding if possible
  RequestAcceptEncoding(true);
}

WSRequest::WSRequest(const URIParser& uri, WS_METHOD method)
: m_port(0)
, m_secure_uri(false)
, m_service_method(method)
, m_charset(REQUEST_STD_CHARSET)
, m_accept()
, m_contentType(WS_CTYPE_Form)
, m_contentTypeStr()
, m_contentData()
{
  if (uri.Host())
    m_server.assign(uri.Host());
  if (uri.Scheme() && strncmp(uri.Scheme(), "https", 5) == 0)
  {
    m_secure_uri = true;
    m_port = uri.Port() ? uri.Port() : 443;
  }
  else
    m_port = uri.Port() ? uri.Port() : 80;

  m_service_url = "/";
  if (uri.Path())
    m_service_url.append(uri.Path());

  if (uri.Fragment())
    m_service_url.append("#").append(uri.Fragment());

  if (uri.Params())
    m_contentData.append(uri.Params());

  // by default allow content encoding if possible
  RequestAcceptEncoding(true);
}

WSRequest::~WSRequest()
{
}

WSRequest::WSRequest(const WSRequest& o, const URIParser& redirection)
: m_server(o.m_server)
, m_port(o.m_port)
, m_secure_uri(o.m_secure_uri)
, m_service_method(o.m_service_method)
, m_charset(o.m_charset)
, m_accept(o.m_accept)
, m_contentType(o.m_contentType)
, m_contentTypeStr(o.m_contentTypeStr)
, m_contentData(o.m_contentData)
, m_headers(o.m_headers)
, m_userAgent(o.m_userAgent)
{
  /* The "Location" header field is used in some responses to refer to a
   * specific resource in relation to the response. The type of relationship
   * is defined by the combination of request method and status code semantics.
   */
  if (redirection.Host())
    m_server.assign(redirection.Host());

  if (redirection.Scheme())
  {
    if (strncmp(redirection.Scheme(), "https", 5) == 0)
    {
      m_secure_uri = true;
      m_port = redirection.Port() ? redirection.Port() : 443;
    }
    else
    {
      m_secure_uri = false;
      m_port = redirection.Port() ? redirection.Port() : 80;
    }
  }

  URIParser o_uri(o.GetService());
  m_service_url = "/";
  if (redirection.Path())
    m_service_url.append(redirection.Path());

  /* If the Location value provided in a 3xx (Redirection) response does not have
   * a fragment component, a user agent MUST process the redirection as if the
   * value inherits the fragment component of the URI reference used to generate
   * the target URI (i.e., the redirection inherits the original reference's
   * fragment, if any).
   */
  if (redirection.Fragment())
    m_service_url.append("#").append(redirection.Fragment());
  else if (o_uri.Fragment())
    m_service_url.append("#").append(o_uri.Fragment());

  /* params have been copied from original request (content data), therefore
   * those specified in the new location are ignored
   */
}

void WSRequest::RequestService(const std::string& url, WS_METHOD method)
{
  m_service_url = url;
  m_service_method = method;
}

void WSRequest::RequestAccept(const std::string& contentType)
{
  m_accept = contentType;
}

void WSRequest::RequestAcceptEncoding(bool yesno)
{
#if HAVE_ZLIB
  if (yesno)
    SetHeader(ws_header_to_str(WS_HEADER_Accept_Encoding), "gzip, deflate");
  else
    SetHeader(ws_header_to_str(WS_HEADER_Accept_Encoding), "");
#else
  (void)yesno;
  SetHeader(ws_header_to_str(WS_HEADER_Accept_Encoding), "");
#endif
}

void WSRequest::SetUserAgent(const std::string& value)
{
  m_userAgent = value;
}

void WSRequest::SetHeader(const std::string& field, const std::string& value)
{
  std::string _key(field);
  std::transform(_key.cbegin(), _key.cend(), _key.begin(), ::toupper);
  m_headers[_key] = std::make_pair(field, value);
}

void  WSRequest::ClearHeader(const std::string& field)
{
  std::string _key(field);
  std::transform(_key.cbegin(), _key.cend(), _key.begin(), ::toupper);
  std::map<std::string, header_t>::const_iterator it = m_headers.find(_key);
  if (it != m_headers.end())
    m_headers.erase(it);
}

void WSRequest::SetContentParam(const std::string& param, const std::string& value)
{
  if (m_contentType != WS_CTYPE_Form)
    return;
  if (!m_contentData.empty())
    m_contentData.append("&");
  m_contentData.append(param).append("=").append(urlencode(value));
}

void WSRequest::SetContentCustom(const std::string& contentType, const char *content)
{
  m_contentType = WS_CTYPE_UNKNOWN;
  m_contentTypeStr = contentType;
  m_contentData = content;
}

void WSRequest::ClearContent()
{
  m_contentData.clear();
  m_contentType = WS_CTYPE_Form;
  m_contentTypeStr.clear();
}

bool WSRequest::WriteMessage(WSRequestStreamSink& sink) const
{
  switch (m_service_method)
  {
  case WS_METHOD_Get:
    return WriteMessageGET(sink, "GET");
  case WS_METHOD_Post:
    return WriteMessagePOST(sink, "POST");
  case WS_METHOD_Head:
    return WriteMessageGET(sink, "HEAD");
  case WS_METHOD_Subscribe:
    return WriteMessageGET(sink, "SUBSCRIBE");
  case WS_METHOD_Unsubscribe:
    return WriteMessageGET(sink, "UNSUBSCRIBE");
  case WS_METHOD_Notify:
    return WriteMessagePOST(sink, "NOTIFY");
  case WS_METHOD_Put:
    return WriteMessagePOST(sink, "PUT");
  case WS_METHOD_Delete:
    return WriteMessageGET(sink, "DELETE");
  case WS_METHOD_Options:
    return WriteMessageGET(sink, "OPTIONS");
  default:
    return false;
  }
}

bool WSRequest::WriteCommonHeading(WSRequestStreamSink& sink) const
{
  BUILTIN_BUFFER buf;
  std::string msg;
  msg.reserve(255);

  // Host
  msg.append(ws_header_to_str(WS_HEADER_Host)).append(": ");
  if (m_server.find(':') == std::string::npos)
    msg.append(m_server);
  else
    msg.append("[").append(m_server).append("]");
  unsigned len = uint_to_strdec(m_port, buf.data, 12, 0);
  msg.append(":").append(buf.data, len).append(WS_CRLF);

  // User-Agent
  if (m_userAgent.empty())
    msg.append(ws_header_to_str(WS_HEADER_User_Agent)).append(": " REQUEST_USER_AGENT WS_CRLF);
  else
    msg.append(ws_header_to_str(WS_HEADER_User_Agent)).append(": ").append(m_userAgent).append(WS_CRLF);

  // Connection
  msg.append(ws_header_to_str(WS_HEADER_Connection)).append(": " REQUEST_CONNECTION WS_CRLF);

  // Accept
  if (!m_accept.empty())
    msg.append(ws_header_to_str(WS_HEADER_Accept)).append(": ").append(m_accept).append(WS_CRLF);

  // Accept-Charset
  msg.append(ws_header_to_str(WS_HEADER_Accept_Charset)).append(": ").append(m_charset).append(WS_CRLF);

  if (!sink.WriteRequestStream(msg.c_str(), msg.size()))
    return false;

  // the rest
  for (std::map<std::string, header_t>::const_iterator it = m_headers.begin(); it != m_headers.end(); ++it)
  {
    msg.assign(it->second.first).append(": ").append(it->second.second).append(WS_CRLF);
    if (!sink.WriteRequestStream(msg.c_str(), msg.size()))
      return false;
  }

  return true;
}

bool WSRequest::WriteMessageGET(WSRequestStreamSink& sink, const char* method) const
{
  std::string msg;
  msg.reserve(127);

  // the request with parameters
  msg.append(method).append(" ").append(m_service_url);
  if (!m_contentData.empty() && m_contentType == WS_CTYPE_Form)
    msg.append("?").append(m_contentData);
  msg.append(" " REQUEST_PROTOCOL WS_CRLF);

  DBG(DBG_PROTO, "%s: %s", __FUNCTION__, msg.c_str());
  if (!sink.WriteRequestStream(msg.c_str(), msg.size()))
    return false;
  if (!WriteCommonHeading(sink))
    return false;
  // close headers
  if (!sink.WriteRequestStream(WS_CRLF, WS_CRLF_LEN))
    return false;

  return sink.FlushRequestStream();
}

bool WSRequest::WriteMessagePOST(WSRequestStreamSink& sink, const char* method) const
{
  std::string msg;
  msg.reserve(127);

  // the request
  msg.append(method).append(" ").append(m_service_url).append(" " REQUEST_PROTOCOL WS_CRLF);

  DBG(DBG_PROTO, "%s: %s", __FUNCTION__, msg.c_str());
  if (!sink.WriteRequestStream(msg.c_str(), msg.size()))
    return false;
  if (!WriteCommonHeading(sink))
    return false;

  if (!m_contentData.empty() && m_contentType != WS_CTYPE_None)
  {
    BUILTIN_BUFFER buf;
    unsigned bl = uint_to_strdec(m_contentData.size(), buf.data, 12, 0);
    msg.clear();
    if (m_contentType == WS_CTYPE_UNKNOWN)
      msg.append(ws_header_to_str(WS_HEADER_Content_Type)).append(": ").append(m_contentTypeStr);
    else
      msg.append(ws_header_to_str(WS_HEADER_Content_Type)).append(": ").append(ws_ctype_to_str(m_contentType));
    msg.append("; charset=" REQUEST_STD_CHARSET WS_CRLF);
    msg.append(ws_header_to_str(WS_HEADER_Content_Length)).append(": ").append(buf.data, bl).append(WS_CRLF);
    // close headers
    msg.append(WS_CRLF);
    if (!sink.WriteRequestStream(msg.c_str(), msg.size()))
      return false;
    // the body
    if (!sink.WriteRequestStream(m_contentData.c_str(), m_contentData.size()))
      return false;
  }
  else
  {
    // close headers
    if (!sink.WriteRequestStream(WS_CRLF, WS_CRLF_LEN))
      return false;
  }

  return sink.FlushRequestStream();
}
