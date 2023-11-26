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
, m_service_method(HRM_GET)
, m_charset(REQUEST_STD_CHARSET)
, m_accept(CT_NONE)
, m_contentType(CT_FORM)
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
, m_service_method(HRM_GET)
, m_charset(REQUEST_STD_CHARSET)
, m_accept(CT_NONE)
, m_contentType(CT_FORM)
, m_contentData()
{
  // by default allow content encoding if possible
  RequestAcceptEncoding(true);
}

WSRequest::WSRequest(const URIParser& uri, HRM_t method)
: m_port(0)
, m_secure_uri(false)
, m_service_method(method)
, m_charset(REQUEST_STD_CHARSET)
, m_accept(CT_NONE)
, m_contentType(CT_FORM)
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

void WSRequest::RequestService(const std::string& url, HRM_t method)
{
  m_service_url = url;
  m_service_method = method;
}

void WSRequest::RequestAccept(CT_t contentType)
{
  m_accept = contentType;
}

void WSRequest::RequestAcceptEncoding(bool yesno)
{
#if HAVE_ZLIB
  if (yesno)
    SetHeader("Accept-Encoding", "gzip, deflate");
  else
    SetHeader("Accept-Encoding", "");
#else
  (void)yesno;
  SetHeader("Accept-Encoding", "");
#endif
}

void WSRequest::SetUserAgent(const std::string& value)
{
  m_userAgent = value;
}

void WSRequest::SetContentParam(const std::string& param, const std::string& value)
{
  if (m_contentType != CT_FORM)
    return;
  if (!m_contentData.empty())
    m_contentData.append("&");
  m_contentData.append(param).append("=").append(urlencode(value));
}

void WSRequest::SetContentCustom(CT_t contentType, const char *content)
{
  m_contentType = contentType;
  m_contentData = content;
}

void WSRequest::SetHeader(const std::string& field, const std::string& value)
{
  m_headers[field] = value;
}

void WSRequest::ClearContent()
{
  m_contentData.clear();
  m_contentType = CT_FORM;
}

void WSRequest::MakeMessage(std::string& msg) const
{
  switch (m_service_method)
  {
  case HRM_GET:
    MakeMessageGET(msg);
    break;
  case HRM_POST:
    MakeMessagePOST(msg);
    break;
  case HRM_HEAD:
    MakeMessageHEAD(msg);
    break;
  case HRM_SUBSCRIBE:
    MakeMessageHEAD(msg, "SUBSCRIBE");
    break;
  case HRM_UNSUBSCRIBE:
    MakeMessageHEAD(msg, "UNSUBSCRIBE");
    break;
  case HRM_NOTIFY:
    MakeMessagePOST(msg, "NOTIFY");
    break;
  default:
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
  msg.append(" " REQUEST_PROTOCOL "\r\n");
  snprintf(buf, sizeof(buf), "%u", m_port);
  msg.append("Host: ").append(m_server).append(":").append(buf).append("\r\n");
  if (m_userAgent.empty())
    msg.append("User-Agent: " REQUEST_USER_AGENT "\r\n");
  else
    msg.append("User-Agent: ").append(m_userAgent).append("\r\n");
  msg.append("Connection: " REQUEST_CONNECTION "\r\n");
  if (m_accept != CT_NONE)
    msg.append("Accept: ").append(MimeFromContentType(m_accept)).append("\r\n");
  msg.append("Accept-Charset: ").append(m_charset).append("\r\n");
  for (std::map<std::string, std::string>::const_iterator it = m_headers.begin(); it != m_headers.end(); ++it)
    msg.append(it->first).append(": ").append(it->second).append("\r\n");
  msg.append("\r\n");
}

void WSRequest::MakeMessagePOST(std::string& msg, const char* method) const
{
  char buf[32];
  size_t content_len = m_contentData.size();

  msg.clear();
  msg.reserve(256);
  msg.append(method).append(" ").append(m_service_url).append(" " REQUEST_PROTOCOL "\r\n");
  snprintf(buf, sizeof(buf), "%u", m_port);
  msg.append("Host: ").append(m_server).append(":").append(buf).append("\r\n");
  if (m_userAgent.empty())
    msg.append("User-Agent: " REQUEST_USER_AGENT "\r\n");
  else
    msg.append("User-Agent: ").append(m_userAgent).append("\r\n");
  msg.append("Connection: " REQUEST_CONNECTION "\r\n");
  if (m_accept != CT_NONE)
    msg.append("Accept: ").append(MimeFromContentType(m_accept)).append("\r\n");
  msg.append("Accept-Charset: ").append(m_charset).append("\r\n");
  if (content_len)
  {
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)content_len);
    msg.append("Content-Type: ").append(MimeFromContentType(m_contentType));
    msg.append("; charset=" REQUEST_STD_CHARSET "\r\n");
    msg.append("Content-Length: ").append(buf).append("\r\n");
  }
  for (std::map<std::string, std::string>::const_iterator it = m_headers.begin(); it != m_headers.end(); ++it)
    msg.append(it->first).append(": ").append(it->second).append("\r\n");
  msg.append("\r\n");
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
  msg.append(" " REQUEST_PROTOCOL "\r\n");
  snprintf(buf, sizeof(buf), "%u", m_port);
  msg.append("Host: ").append(m_server).append(":").append(buf).append("\r\n");
  if (m_userAgent.empty())
    msg.append("User-Agent: " REQUEST_USER_AGENT "\r\n");
  else
    msg.append("User-Agent: ").append(m_userAgent).append("\r\n");
  msg.append("Connection: " REQUEST_CONNECTION "\r\n");
  if (m_accept != CT_NONE)
    msg.append("Accept: ").append(MimeFromContentType(m_accept)).append("\r\n");
  msg.append("Accept-Charset: ").append(m_charset).append("\r\n");
  for (std::map<std::string, std::string>::const_iterator it = m_headers.begin(); it != m_headers.end(); ++it)
    msg.append(it->first).append(": ").append(it->second).append("\r\n");
  msg.append("\r\n");
}
