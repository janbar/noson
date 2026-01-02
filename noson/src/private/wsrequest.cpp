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

#include "wsrequest.h"
#include "urlencoder.h"
#include "debug.h"

#include <cstdio>
#include <cstring> // for strlen

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

void WSRequest::SetHeader(const std::string& field, const std::string& value)
{
  m_headers[field] = value;
}

void WSRequest::ClearContent()
{
  m_contentData.clear();
  m_contentType = WS_CTYPE_Form;
  m_contentTypeStr.clear();
}

void WSRequest::MakeMessage(std::string& msg) const
{
  switch (m_service_method)
  {
  case WS_METHOD_Get:
    MakeMessageGET(msg);
    break;
  case WS_METHOD_Post:
    MakeMessagePOST(msg);
    break;
  case WS_METHOD_Head:
    MakeMessageHEAD(msg);
    break;
  case WS_METHOD_Subscribe:
    MakeMessageHEAD(msg, "SUBSCRIBE");
    break;
  case WS_METHOD_Unsubscribe:
    MakeMessageHEAD(msg, "UNSUBSCRIBE");
    break;
  case WS_METHOD_Notify:
    MakeMessagePOST(msg, "NOTIFY");
    break;
  case WS_METHOD_Put:
    MakeMessagePOST(msg, "PUT");
    break;
  case WS_METHOD_Delete:
    MakeMessageHEAD(msg, "DELETE");
    break;
  case WS_METHOD_Options:
    MakeMessageHEAD(msg, "OPTIONS");
    break;
  case WS_METHOD_UNKNOWN:
    break;
  }
}

void WSRequest::MakeMessageGET(std::string& msg, const char* method) const
{
  char buf[32];

  msg.clear();
  msg.reserve(256);
  msg.append(method).append(" ").append(m_service_url);
  if (!m_contentData.empty())
    msg.append("?").append(m_contentData);
  msg.append(" " REQUEST_PROTOCOL WS_CRLF);
  snprintf(buf, sizeof(buf), "%u", m_port);
  msg.append(ws_header_to_str(WS_HEADER_Host)).append(": ").append(m_server).append(":").append(buf).append(WS_CRLF);
  if (m_userAgent.empty())
    msg.append(ws_header_to_str(WS_HEADER_User_Agent)).append(": " REQUEST_USER_AGENT WS_CRLF);
  else
    msg.append(ws_header_to_str(WS_HEADER_User_Agent)).append(": ").append(m_userAgent).append(WS_CRLF);
  msg.append(ws_header_to_str(WS_HEADER_Connection)).append(": " REQUEST_CONNECTION WS_CRLF);
  if (!m_accept.empty())
    msg.append(ws_header_to_str(WS_HEADER_Accept)).append(": ").append(m_accept).append(WS_CRLF);
  msg.append(ws_header_to_str(WS_HEADER_Accept_Charset)).append(": ").append(m_charset).append(WS_CRLF);
  for (std::map<std::string, std::string>::const_iterator it = m_headers.begin(); it != m_headers.end(); ++it)
    msg.append(it->first).append(": ").append(it->second).append(WS_CRLF);
  msg.append(WS_CRLF);
}

void WSRequest::MakeMessagePOST(std::string& msg, const char* method) const
{
  char buf[32];
  size_t content_len = m_contentData.size();

  msg.clear();
  msg.reserve(256);
  msg.append(method).append(" ").append(m_service_url).append(" " REQUEST_PROTOCOL WS_CRLF);
  snprintf(buf, sizeof(buf), "%u", m_port);
  msg.append(ws_header_to_str(WS_HEADER_Host)).append(": ").append(m_server).append(":").append(buf).append(WS_CRLF);
  if (m_userAgent.empty())
    msg.append(ws_header_to_str(WS_HEADER_User_Agent)).append(": " REQUEST_USER_AGENT WS_CRLF);
  else
    msg.append(ws_header_to_str(WS_HEADER_User_Agent)).append(": ").append(m_userAgent).append(WS_CRLF);
  msg.append(ws_header_to_str(WS_HEADER_Connection)).append(": " REQUEST_CONNECTION WS_CRLF);
  if (!m_accept.empty())
    msg.append(ws_header_to_str(WS_HEADER_Accept)).append(": ").append(m_accept).append(WS_CRLF);
  msg.append(ws_header_to_str(WS_HEADER_Accept_Charset)).append(": ").append(m_charset).append(WS_CRLF);
  if (m_contentType != WS_CTYPE_None && content_len)
  {
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)content_len);
    if (m_contentType == WS_CTYPE_UNKNOWN)
      msg.append(ws_header_to_str(WS_HEADER_Content_Type)).append(": ").append(m_contentTypeStr);
    else
      msg.append(ws_header_to_str(WS_HEADER_Content_Type)).append(": ").append(ws_ctype_to_str(m_contentType));
    msg.append("; charset=" REQUEST_STD_CHARSET WS_CRLF);
    msg.append(ws_header_to_str(WS_HEADER_Content_Length)).append(": ").append(buf).append(WS_CRLF);
  }
  for (std::map<std::string, std::string>::const_iterator it = m_headers.begin(); it != m_headers.end(); ++it)
    msg.append(it->first).append(": ").append(it->second).append(WS_CRLF);
  msg.append(WS_CRLF);
  if (content_len)
    msg.append(m_contentData);
}

void WSRequest::MakeMessageHEAD(std::string& msg, const char* method) const
{
  char buf[32];

  msg.clear();
  msg.reserve(256);
  msg.append(method).append(" ").append(m_service_url);
  if (!m_contentData.empty())
    msg.append("?").append(m_contentData);
  msg.append(" " REQUEST_PROTOCOL WS_CRLF);
  snprintf(buf, sizeof(buf), "%u", m_port);
  msg.append(ws_header_to_str(WS_HEADER_Host)).append(": ").append(m_server).append(":").append(buf).append(WS_CRLF);
  if (m_userAgent.empty())
    msg.append(ws_header_to_str(WS_HEADER_User_Agent)).append(": " REQUEST_USER_AGENT WS_CRLF);
  else
    msg.append(ws_header_to_str(WS_HEADER_User_Agent)).append(": ").append(m_userAgent).append(WS_CRLF);
  msg.append(ws_header_to_str(WS_HEADER_Connection)).append(": " REQUEST_CONNECTION WS_CRLF);
  if (!m_accept.empty())
    msg.append(ws_header_to_str(WS_HEADER_Accept)).append(": ").append(m_accept).append(WS_CRLF);
  msg.append(ws_header_to_str(WS_HEADER_Accept_Charset)).append(": ").append(m_charset).append(WS_CRLF);
  for (std::map<std::string, std::string>::const_iterator it = m_headers.begin(); it != m_headers.end(); ++it)
    msg.append(it->first).append(": ").append(it->second).append(WS_CRLF);
  msg.append(WS_CRLF);
}
