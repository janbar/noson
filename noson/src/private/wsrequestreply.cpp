/*
 *      Copyright (C) 2026 Jean-Luc Barriere
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

#include "wsrequestreply.h"
#include "wsreplychunked.h"
#include "builtin.h"
#include "debug.h"
#include "wsstatic.h"

#include <algorithm>

using namespace NSROOT;

WSRequestReply::WSRequestReply(WSRequestBroker& rb)
: m_broker(rb)
, m_stage(STAGE_HEADER)
, m_chunked(nullptr)
{
  (void)ResetReply();
}

WSRequestReply::~WSRequestReply()
{

}

void WSRequestReply::AddHeader(const std::string& headerStr, const std::string& str, bool encap /*= false*/)
{
  std::string tmp;
  tmp.reserve(headerStr.size() + str.size() + 2);
  tmp.assign(headerStr).append(": ");
  if (!encap)
    tmp.append(str);
  else
  {
    tmp.push_back('\"');
    for (auto& c : str)
    {
      if (c == '"')
        tmp.push_back('\\');
      tmp.push_back(c);
    }
    tmp.push_back('\"');
  }
  tmp.append(WS_CRLF);
  m_headers.insert(std::make_pair(headerStr, tmp));
}

void WSRequestReply::AddHeader(WS_HEADER header, const std::string& str, bool encap /*= false*/)
{
  const char* hs = ws_header_to_str(header);
  AddHeader(hs, str, encap);
}

void WSRequestReply::AddHeader(WS_HEADER header, uint32_t num)
{
  BUILTIN_BUFFER str;
  uint32_to_string(num, &str);
  AddHeader(header, str.data);
}

bool WSRequestReply::ResetReply()
{
  if (m_stage != STAGE_HEADER)
  {
    DBG(DBG_ERROR, "%s: bad stage (%d)\n", __FUNCTION__, m_stage);
    return false;
  }
  m_headers.clear();
  AddHeader(WS_HEADER_Server, REQUEST_USER_AGENT);
  AddHeader(WS_HEADER_Connection, "close");
  return true;
}

bool WSRequestReply::PostReply(WS_STATUS status)
{
  if (m_stage != STAGE_HEADER)
  {
    DBG(DBG_ERROR, "%s: bad stage (%d)\n", __FUNCTION__, m_stage);
    return false;
  }
  if (status == WS_STATUS_UNKNOWN)
  {
    DBG(DBG_ERROR, "%s: invalid status\n", __FUNCTION__);
    return false;
  }
  m_stage = STAGE_CLOSE;
  m_broker.SetStatus(status);
  std::string data;
  data.reserve(64);
  data.append(REQUEST_PROTOCOL " ")
      .append(ws_status_to_numstr(status))
      .append(" ")
      .append(ws_status_to_msgstr(status))
      .append(WS_CRLF);
  if (!m_broker.ReplyData(data.c_str(), data.size()))
    return false;
  for (auto& e : m_headers)
  {
    if (!m_broker.ReplyData(e.second.c_str(), e.second.size()))
      return false;
  }
  return m_broker.ReplyData(WS_CRLF, WS_CRLF_LEN);
}

bool WSRequestReply::BeginContent(WS_STATUS status, int chunkSize)
{
  AddHeader(WS_HEADER_Transfer_Encoding, "chunked");
  if (!PostReply(status))
    return false;
  m_chunked = new WSReplyChunked(m_broker, chunkSize);
  m_stage = STAGE_CONTENT;
  m_broker.SetStatus(status);
  return true;
}

bool WSRequestReply::WriteData(const char* data, int len)
{
  if (m_stage != STAGE_CONTENT)
  {
    DBG(DBG_ERROR, "%s: bad stage (%d)\n", __FUNCTION__, m_stage);
    return false;
  }
  if (m_chunked->Write(data, len) == len)
    return true;
  return false;
}

int WSRequestReply::WriteFileStream(FILE* file)
{
  if (m_stage != STAGE_CONTENT)
  {
    DBG(DBG_ERROR, "%s: bad stage (%d)\n", __FUNCTION__, m_stage);
    return false;
  }
  return m_chunked->ReadFileStream(file);
}

int WSRequestReply::WriteFileStream(FILE* file, unsigned maxlen)
{
  if (m_stage != STAGE_CONTENT)
  {
    DBG(DBG_ERROR, "%s: bad stage (%d)\n", __FUNCTION__, m_stage);
    return false;
  }
  return m_chunked->ReadFileStream(file, maxlen);
}

int WSRequestReply::WriteInputStream(InputStream& in)
{
  if (m_stage != STAGE_CONTENT)
  {
    DBG(DBG_ERROR, "%s: bad stage (%d)\n", __FUNCTION__, m_stage);
    return false;
  }
  return m_chunked->ReadInputStream(in);
}

bool WSRequestReply::CloseContent()
{
  if (m_stage == STAGE_CLOSE)
    return true;
  if (m_stage != STAGE_CONTENT)
    return false;
  m_stage = STAGE_CLOSE;
  if (!m_chunked->Flush())
    DBG(DBG_ERROR, "%s: chunk flush failed\n", __FUNCTION__);
  delete m_chunked;
  m_chunked = nullptr;
  return true;
}

WSRequestReply::STAGE WSRequestReply::Abort()
{
   STAGE s = m_stage;
   m_stage = STAGE_CLOSE;
   return s;
}
