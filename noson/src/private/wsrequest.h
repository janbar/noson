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

#ifndef WSREQUEST_H
#define	WSREQUEST_H

#include "local_config.h"
#include "wsstatic.h"
#include "uriparser.h"

#include <cstddef>  // for size_t
#include <string>
#include <map>

#define REQUEST_PROTOCOL      "HTTP/1.1"
#define REQUEST_USER_AGENT    LIBTAG "/" LIBVERSION
#define REQUEST_CONNECTION    "close"
#define REQUEST_STD_CHARSET   "utf-8"

namespace NSROOT
{

  class WSRequest
  {
  public:
    WSRequest(const std::string& server, unsigned port);
    WSRequest(const std::string& server, unsigned port, bool secureURI);
    WSRequest(const URIParser& uri, WS_METHOD method = WS_METHOD_Get);
    ~WSRequest();

    // Clone for redirection: see RFC-9110 section 10.2.2 Location
    WSRequest(const WSRequest& o, const URIParser& redirection);

    void RequestService(const std::string& url, WS_METHOD method = WS_METHOD_Get);
    void RequestAccept(const std::string& contentType);
    void RequestAcceptEncoding(bool yesno);
    void SetUserAgent(const std::string& value);
    void SetContentParam(const std::string& param, const std::string& value);
    void SetContentCustom(const std::string& contentType, const char *content);
    void SetHeader(const std::string& field, const std::string& value);
    const std::string& GetContent() const { return m_contentData; }
    void ClearContent();

    void MakeMessage(std::string& msg) const;

    const std::string& GetServer() const { return m_server; }
    unsigned GetPort() const { return m_port; }
    bool IsSecureURI() const { return m_secure_uri; }
    WS_METHOD GetMethod() const { return m_service_method; }
    const std::string& GetService() const { return m_service_url; }

  private:
    std::string m_server;
    unsigned m_port;
    bool m_secure_uri;
    std::string m_service_url;
    WS_METHOD m_service_method;
    std::string m_charset;
    std::string m_accept;
    WS_CTYPE m_contentType;
    std::string m_contentTypeStr;
    std::string m_contentData;
    std::map<std::string, std::string> m_headers;
    std::string m_userAgent;

    void MakeMessageGET(std::string& msg, const char* method = "GET") const;
    void MakeMessagePOST(std::string& msg, const char* method = "POST") const;
    void MakeMessageHEAD(std::string& msg, const char* method = "HEAD") const;
  };

}

#endif	/* WSREQUEST_H */
