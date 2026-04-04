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

#include <algorithm>

using namespace NSROOT;

namespace
{
std::string encap_str(const std::string& str)
{
  std::string val;
  val.reserve(str.size() + sizeof(long));
  val.push_back('\"');
  for (auto& c : str)
  {
    if (c == '"')
      val.push_back('\\');
    val.push_back(c);
  }
  val.push_back('\"');
  return val;
}
}

WSRequestReply::WSRequestReply(WSRequestBroker& rb)
: m_broker(rb)
, m_stage(STAGE_HEADER)
, m_chunked(nullptr)
{
  (void)ResetReply();
}

WSRequestReply::~WSRequestReply()
{
  if (m_chunked)
    delete m_chunked;
}

void WSRequestReply::SetHeader(WS_HEADER header, const std::string& str, bool encap /*= false*/)
{
  const char* key = ws_header_to_upperstr(header);
  if (str.empty())
  {
    m_headers.erase(key);
    return;
  }
  WSHeader& h = m_headers[key];
  h.SetName(ws_header_to_str(header));
  if (encap)
    h.SetValue(encap_str(str));
  else
    h.SetValue(str);
}

void WSRequestReply::SetHeader(WS_HEADER header, uint32_t num)
{
  BUILTIN_BUFFER str;
  uint32_to_string(num, &str);
  SetHeader(header, str.data);
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

void WSRequestReply::AddHeader(const std::string& headerStr, const std::string& str, bool encap /*= false*/)
{
  if (str.empty() || headerStr.empty())
    return;
  std::string key(headerStr);
  std::transform(key.begin(), key.end(), key.begin(), ::toupper);
  WSHeader& h = m_headers[key];
  h.SetName(headerStr);
  if (encap)
    h.AddValue(encap_str(str));
  else
    h.AddValue(str);
}

bool WSRequestReply::ResetReply()
{
  if (m_stage != STAGE_HEADER)
  {
    DBG(DBG_ERROR, "%s: bad stage (%d)\n", __FUNCTION__, m_stage);
    return false;
  }
  m_headers.clear();
  SetHeader(WS_HEADER_Server, SERVER_SOFTWARE);
  SetHeader(WS_HEADER_Connection, SERVER_CONNECTION);
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
  data.reserve(127);
  data.append(SERVER_PROTOCOL " ")
      .append(ws_status_to_numstr(status))
      .append(" ")
      .append(ws_status_to_msgstr(status))
      .append(WS_CRLF);
  if (!m_broker.ReplyData(data.c_str(), data.size()))
    return false;
  for (auto& e : m_headers)
  {
    for (auto it = e.second.cbegin(); it != e.second.cend(); ++it)
    {
      data.assign(e.second.Name()).append(": ").append(*it);
      data.append(WS_CRLF);
      if (!m_broker.ReplyData(data.c_str(), data.size()))
        return false;
    }
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

bool WSRequestReply::WriteString(const char* str)
{
  if (m_stage != STAGE_CONTENT)
  {
    DBG(DBG_ERROR, "%s: bad stage (%d)\n", __FUNCTION__, m_stage);
    return false;
  }
  return m_chunked->Write(str, strlen(str));
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

void WSRequestReply::ReturnStatus(WSRequestBroker& rb, WS_STATUS status)
{
  // build the status page
  std::string content;
  content.reserve(255);
  content.append("<!DOCTYPE html><html lang=\"en\"><head><title>");
  content.append(ws_status_to_numstr(status));
  content.append("</title></head><body><h1 style=\"text-align: center;\">");
  content.append(ws_status_to_numstr(status)).append(" ");
  content.append(ws_status_to_msgstr(status));
  content.append("</h1></body></html>");
  // make basic reply without encoding
  WSRequestReply rr(rb);
  rr.AddHeader(WS_HEADER_Content_Type, "text/html");
  rr.AddHeader(WS_HEADER_Content_Length, (uint32_t)content.size());
  if (rr.PostReply(status))
    rb.ReplyData(content.c_str(), content.size());
}
