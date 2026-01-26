/*
 *      Copyright (C) 2019 Jean-Luc Barriere
 *
 *  This file is part of Noson
 *
 *  Noson is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Noson is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "requestbroker.h"
#include "private/requestbrokeropaque.h"
#include "private/socket.h"
#include "private/wsrequestbroker.h"
#include "private/wsstatic.h"
#include <cassert>

using namespace NSROOT;

RequestBroker::RequestBroker()
: m_aborted(false)
, m_200(0)
, m_400(0)
, m_404(0)
, m_429(0)
, m_500(0)
, m_503(0)
{
}

RequestBroker::~RequestBroker()
{
}

bool RequestBroker::Initialize() { return true; }

std::string RequestBroker::MakeResponseHeader(Status status)
{
  std::string header;
  WS_STATUS hsc = WS_STATUS_500_Internal_Server_Error;
  switch (status)
  {
  case Status_OK: hsc = WS_STATUS_200_OK; m_200.Increment(); break;
  case Status_Partial_Content: hsc = WS_STATUS_206_Partial_Content; m_200.Increment(); break;
  case Status_Bad_Request: hsc = WS_STATUS_400_Bad_Request; m_400.Increment(); break;
  case Status_Not_Found: hsc = WS_STATUS_404_Not_Found; m_404.Increment(); break;
  case Status_Too_Many_Requests: hsc = WS_STATUS_429_Too_Many_Requests; m_429.Increment(); break;
  case Status_Internal_Server_Error: hsc = WS_STATUS_500_Internal_Server_Error; m_500.Increment(); break;
  case Status_Service_Unavailable: hsc = WS_STATUS_503_Service_Unavailable; m_503.Increment(); break;
  case Status_Range_Not_Satisfiable: hsc = WS_STATUS_416_Range_Not_Satisfiable; m_400.Increment(); break;
  }
  header.append(REQUEST_PROTOCOL " ").append(ws_status_to_numstr(hsc)).append(" ").append(ws_status_to_msgstr(hsc)).append(WS_CRLF);
  header.append("Server: ").append(REQUEST_USER_AGENT).append(WS_CRLF);
  header.append("Connection: close" WS_CRLF);
  return header;
}

bool RequestBroker::Reply(handle * handle, const char* data, size_t size)
{
  assert(handle);
  return handle->payload->socket->SendData(data, size);
}

RequestBroker::Method RequestBroker::GetRequestMethod(handle * handle)
{
  assert(handle);
  switch (handle->payload->request->GetRequestMethod())
  {
  case WS_METHOD_Get:
    return Method_GET;
  case WS_METHOD_Post:
    return Method_POST;
  case WS_METHOD_Head:
    return Method_HEAD;
  case WS_METHOD_Subscribe:
    return Method_SUBSCRIBE;
  case WS_METHOD_Unsubscribe:
    return Method_UNSUBSCRIBE;
  case WS_METHOD_Notify:
    return Method_NOTIFY;
  default:
    return Method_UNKNOWN;
  }
}

const std::string& RequestBroker::GetRequestPath(handle * handle)
{
  assert(handle);
  return handle->payload->request->GetRequestPath();
}

const std::string& RequestBroker::GetURIParams(handle * handle)
{
  assert(handle);
  return handle->payload->request->GetURIParams();
}

const std::string& RequestBroker::GetRequestProtocol(handle * handle)
{
  assert(handle);
  return handle->payload->request->GetRequestProtocol();
}

const std::string& RequestBroker::GetRequestHeader(handle * handle, const std::string& name)
{
  assert(handle);
  return handle->payload->request->GetRequestHeader(name);
}

bool RequestBroker::HasContent(handle * handle)
{
  assert(handle);
  return handle->payload->request->HasContent();
}

size_t RequestBroker::GetContentLength(handle * handle)
{
  assert(handle);
  return handle->payload->request->GetContentLength();
}

size_t RequestBroker::GetConsumed(handle * handle)
{
  assert(handle);
  return handle->payload->request->GetConsumed();
}

size_t RequestBroker::ReadContent(handle * handle, std::string& data)
{
  assert(handle);
  WSRequestBroker * request = handle->payload->request;
  size_t len = 0, l = 0;
  char buffer[4096];
  while ((l = request->ReadContent(buffer, sizeof(buffer))))
  {
    data.append(buffer, l);
    len += l;
  }
  return len;
}

std::string RequestBroker::buildDelegateUrl(const RequestBroker::Resource& res, const std::string& params)
{
  if (params.empty())
    return res.sourcePath;
  return res.sourcePath + "?" + params;
}

std::string RequestBroker::buildUri(const std::string &rootUri, const std::string &path)
{
  unsigned rpath = 0;
  while (rpath < path.length() && path.at(rpath) == '/') ++rpath;
  return std::string(rootUri).append(path.substr(rpath));
}

RequestBroker::Resource::Resource()
: uri()
, title()
, description()
, contentType("application/octet-stream")
, iconUri()
, sourcePath()
, delegate(nullptr)
{
}
