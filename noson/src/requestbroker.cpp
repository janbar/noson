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

void RequestBroker::TraceResponseStatus(int status)
{
  switch (ws_status_from_num(status))
  {
  case WS_STATUS_200_OK: m_200.Increment(); break;
  case WS_STATUS_206_Partial_Content: m_200.Increment(); break;
  case WS_STATUS_400_Bad_Request: m_400.Increment(); break;
  case WS_STATUS_404_Not_Found: m_404.Increment(); break;
  case WS_STATUS_429_Too_Many_Requests: m_429.Increment(); break;
  case WS_STATUS_500_Internal_Server_Error: m_500.Increment(); break;
  case WS_STATUS_503_Service_Unavailable: m_503.Increment(); break;
  case WS_STATUS_416_Range_Not_Satisfiable: m_400.Increment(); break;
  default: break;
  }
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
