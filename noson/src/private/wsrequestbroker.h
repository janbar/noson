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

#ifndef WSREQUESTBROKER_H
#define	WSREQUESTBROKER_H

#include "local_config.h"
#include "os/os.h"
#include "wsresponse.h"

#include <string>
#include <vector>
#include <map>

namespace NSROOT
{

  class TcpSocket;

  class WSRequestBroker
  {
  public:
    WSRequestBroker(TcpSocket* socket, bool secure, int timeout);
    ~WSRequestBroker();

    void SetTimeout(int timeout);
    bool IsSecure() const { return m_secure; }
    bool IsParsed() const { return m_parsed; }
    std::string GetHostAddrInfo() const;
    WS_METHOD GetRequestMethod() const { return m_method; }
    const std::string& GetRequestPath() const { return m_path; }
    const std::string& GetRequestProtocol() const { return m_protocol; }
    const std::string& GetRequestHeader(const std::string& name) const;
    const std::string& GetRequestHeader(WS_HEADER header) const { return GetRequestHeader(ws_header_to_upperstr(header)); }
    const std::string& GetRequestParam(const std::string& name) const;
    const std::string& GetURIParams() const { return m_uriParams; }
    bool IsPathHidden() const { return m_pathIsHidden; }
    bool HasContent() const { return (m_contentLength > 0); }
    size_t GetContentLength() const { return m_contentLength; }
    size_t ReadContent(char *buf, size_t buflen);
    size_t GetConsumed() const { return m_consumed; }

    static void Tokenize(const std::string& str, char delimiter, std::vector<std::string>& tokens, bool trimnull = false);
    static bool ExplodeURI(const std::string& in, std::string& path, std::string& uriparams, bool& ishidden);

    bool ReplyHead(WS_STATUS status);
    bool ReplyBody(const char * data, size_t size) const;
    bool RewritePath(const std::string& newpath);

  private:
    TcpSocket* m_socket;
    bool m_secure;
    bool m_parsed;
    WS_METHOD m_method;
    std::string m_path;
    std::string m_protocol;
    std::string m_uriParams;
    bool m_pathIsHidden;
    bool m_contentChunked;
    size_t m_contentLength;
    size_t m_consumed;
    char* m_chunkBuffer;
    char* m_chunkPtr;
    char* m_chunkEnd;

    typedef std::map<std::string, std::string> VARS;
    VARS m_requestHeaders;
    VARS m_requestParams;

    // prevent copy
    WSRequestBroker(const WSRequestBroker&);
    WSRequestBroker& operator=(const WSRequestBroker&);

    bool ReadHeaderLine(const char *eol, std::string& line, size_t *len);
    bool ParseQuery();
    size_t ReadChunk(void *buf, size_t buflen);
  };

}

#endif	/* WSREQUESTBROKER_H */

