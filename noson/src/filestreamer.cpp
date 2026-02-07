/*
 *      Copyright (C) 2018-2019 Jean-Luc Barriere
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "filestreamer.h"
#include "imageservice.h"
#include "data/datareader.h"
#include "private/debug.h"
#include "private/urlencoder.h"
#include "private/tokenizer.h"
#include "private/wsstatic.h"
#include "private/wsrequestbroker.h"
#include "private/wsrequestreply.h"

#include <cstring>
#include <cstdio>
#include <cassert>
#include <cinttypes>
#include <climits>

#define FILESTREAMER_TIMEOUT  10000
#define FILESTREAMER_MAX_PB   5
#define FILESTREAMER_CHUNK    16384

using namespace NSROOT;

FileStreamer::codec_type FileStreamer::codecTypeTab[] = {
  { "flac"          , "flac" , "audio/flac" },
  { "mpeg1layer3"   , "mp3"  , "audio/mpeg" },
  { "mpeg2layer3"   , "mp3"  , "audio/mpeg" },
  { "mpeg1layer2"   , "mp2"  , "audio/mpeg" },
  { "mpeg2layer2"   , "mp2"  , "audio/mpeg" },
  { "mpeg2aac"      , "aac"  , "audio/aac"  },
  { "mpeg4aac"      , "aac"  , "audio/aac"  },
  { "vorbis"        , "ogg"  , "application/ogg" },
  { "mp4a"          , "m4a"  , "audio/mp4"  },
  { "mp4b"          , "m4b"  , "audio/mp4"  },
};

int FileStreamer::codecTypeTabSize = sizeof(FileStreamer::codecTypeTab) / sizeof(FileStreamer::codec_type);

FileStreamer::file_type FileStreamer::fileTypeTab[] = {
  { "audio/flac"        , &FileStreamer::probeFLAC },
  { "audio/mpeg"        , &FileStreamer::probeMPEG },
  { "audio/aac"         , &FileStreamer::probeMPEG },
  { "application/ogg"   , &FileStreamer::probeOGGS },
  { "audio/mp4"         , &FileStreamer::probeMP4A },
};

int FileStreamer::fileTypeTabSize = sizeof(FileStreamer::fileTypeTab) / sizeof(FileStreamer::file_type);

FileStreamer::FileStreamer()
: SONOS::RequestBroker()
, m_resources()
, m_playbackCount(0)
{
  // declare the static resources for available codecs
  for (int i = 0; i < codecTypeTabSize; ++i)
  {
    ResourcePtr ptr = ResourcePtr(new Resource());
    ptr->uri.assign(FILESTREAMER_URI).append(".").append(codecTypeTab[i].suffix);
    ptr->title = codecTypeTab[i].codec;
    ptr->description = "Stream file";
    ptr->contentType = codecTypeTab[i].mime;
    m_resources.push_back(ptr);
  }
}


bool FileStreamer::HandleRequest(handle * handle)
{
  const std::string& requrl = handle->broker->GetRequestPath();
  ResourceList::const_iterator it = m_resources.begin();
  while (!IsAborted() && it != m_resources.end())
  {
    if (requrl.compare(0, (*it)->uri.length(), (*it)->uri) == 0)
    {
      std::vector<std::string> params;
      tokenize(handle->broker->GetURIParams(), "&", "", params, true);
      std::string filePath = getParamValue(params, FILESTREAMER_PARAM_PATH);
      if (probe(filePath, (*it)->contentType))
      {
        switch (handle->broker->GetRequestMethod())
        {
        case WS_METHOD_Get:
          if (handle->broker->GetRequestHeader("RANGE").empty())
            streamFile(handle, filePath, (*it)->contentType);
          else
            streamFileRange(handle, filePath, (*it)->contentType, handle->broker->GetRequestHeader("RANGE"));
          return true;
        case WS_METHOD_Head:
        {
          TraceResponseStatus(200);
          WSRequestReply reply(*handle->broker);
          reply.AddHeader(WS_HEADER_Content_Type, (*it)->contentType);
          reply.AddHeader(WS_HEADER_Accept_Ranges, "bytes");
          reply.AddHeader(WS_HEADER_Content_Length, std::to_string(getFileLength(filePath)));
          reply.PostReply(WS_STATUS_200_OK);
          return true;
        }
        default:
          return false; // unhandled method
        }
      }
      else
      {
        DBG(DBG_WARN, "%s: probing file failed (%s)\n", __FUNCTION__, filePath.c_str());
        TraceResponseStatus(500);
        WSRequestReply reply(*handle->broker);
        reply.PostReply(WS_STATUS_500_Internal_Server_Error);
      }
      return true;
    }
    ++it;
  }
  return false;
}

RequestBroker::ResourcePtr FileStreamer::GetResource(const std::string& title)
{
  for (ResourceList::iterator it = m_resources.begin(); it != m_resources.end(); ++it)
  {
    if ((*it)->title == title)
      return (*it);
  }
  return ResourcePtr();
}

RequestBroker::ResourceList FileStreamer::GetResourceList()
{
  ResourceList list;
  for (ResourceList::iterator it = m_resources.begin(); it != m_resources.end(); ++it)
    list.push_back((*it));
  return list;
}

RequestBroker::ResourcePtr FileStreamer::RegisterResource(const std::string& title,
                                                           const std::string& description,
                                                           const std::string& path,
                                                           StreamReader * delegate)
{
  (void)title;
  (void)description;
  (void)path;
  (void)delegate;
  return ResourcePtr();
}

void FileStreamer::UnregisterResource(const std::string& uri)
{
  (void)uri;
}

const FileStreamer::codec_type * FileStreamer::GetCodec(const std::string& codecName)
{
  for (int i = 0; i < codecTypeTabSize; ++i)
    if (codecName.compare(codecTypeTab[i].codec) == 0)
      return &codecTypeTab[i];
  return nullptr;
}

std::string FileStreamer::MakeFileStreamURI(const std::string& filePath, const std::string& codecName)
{
  std::string streamUri;
  // find the resource for my codec else return null
  ResourcePtr res = GetResource(codecName);
  if (!res)
    return streamUri;
  // encode the file path
  std::string pathParm(urlencode(filePath));
  // make the stream uri
  if (res->uri.find('?') != std::string::npos)
    streamUri.assign(res->uri).append("&path=").append(pathParm);
  else
    streamUri.assign(res->uri).append("?path=").append(pathParm);

  return streamUri;
}

std::string FileStreamer::getParamValue(const std::vector<std::string>& params, const std::string& name)
{
  size_t lval = name.length() + 1;
  for (const std::string& str : params)
  {
    if (str.length() > lval && str.at(name.length()) == '=' && str.compare(0, name.length(), name) == 0)
      return urldecode(str.substr(lval));
  }
  return std::string();
}

size_t FileStreamer::getFileLength(FILE * file)
{
  size_t ret = 0;
  if (file)
  {
    int64_t p = 0;
    int64_t c = ftell(file);
    if (c >= 0)
    {
      if (fseek(file, 0, SEEK_END) == 0 && (p = ftell(file)) > 0)
        ret = (size_t)p;
      fseek(file, c, SEEK_SET);
    }
  }
  return ret;
}

size_t FileStreamer::getFileLength(const std::string& filePath)
{
  size_t ret = 0;
  FILE * file = fopen(filePath.c_str(), "rb");
  if (file)
  {
    ret = getFileLength(file);
    fclose(file);
  }
  return ret;
}

bool FileStreamer::probe(const std::string& filePath, const std::string& mimeType)
{
  for (int i = 0; i < fileTypeTabSize; ++i)
  {
    if (mimeType.compare(fileTypeTab[i].mime) == 0 && fileTypeTab[i].probe(filePath))
      return true;
  }
  return false;
}

bool FileStreamer::probeFLAC(const std::string& filePath)
{
  bool ret = false;
  FILE * file = fopen(filePath.c_str(), "rb");
  if (file)
  {
    char buf[4];
    if (fread(buf, 1, 4, file) == 4 && memcmp(buf, "fLaC", 4) == 0)
      ret = true;
    fclose(file);
  }
  return ret;
}

bool FileStreamer::probeMPEG(const std::string& filePath)
{
  bool ret = false;
  FILE * file = fopen(filePath.c_str(), "rb");
  if (file)
  {
    unsigned char buf[10];
    if (fread(buf, 1, 10, file) == 10)
    {
      // unstack id3 tags
      while (memcmp(buf, "ID3", 3) == 0)
      {
        unsigned offset = 0;
        for (int i = 6; i < 10; ++i)
          offset |= buf[i] << ((9 - i) * 7);
        if ((buf[5] & 0x10) == 0x10) // check for extended footer
          offset += 10;
        if (!offset || fseek(file, offset, SEEK_CUR) != 0 || fread(buf, 1, 10, file) != 10)
        {
          fclose(file);
          return false;
        }
      }
      // check mpeg synchro
      if (buf[0] == 0xff)
      {
        unsigned type = buf[1] & 0xfe;
        if (type == 0xfa || // version 1 layer 3
            type == 0xfc || // version 1 layer 2
            type == 0xf2 || // version 2 layer 3
            type == 0xf4 || // version 2 layer 2
            type == 0xf8 || // version 2 aac
            type == 0xf0)   // version 4 aac
          ret = true;
      }
    }
    fclose(file);
  }
  return ret;
}

bool FileStreamer::probeOGGS(const std::string& filePath)
{
  bool ret = false;
  FILE * file = fopen(filePath.c_str(), "rb");
  if (file)
  {
    char buf[4];
    if (fread(buf, 1, 4, file) == 4 && memcmp(buf, "OggS", 4) == 0)
      ret = true;
    fclose(file);
  }
  return ret;
}

bool FileStreamer::probeMP4A(const std::string& filePath)
{
  bool ret = false;
  FILE * file = fopen(filePath.c_str(), "rb");
  if (file)
  {
    char buf[12];
    if (fread(buf, 1, 12, file) == 12 &&
            (memcmp(buf + 4, "ftypM4A ", 8) == 0 || memcmp(buf + 4, "ftypM4B ", 8) == 0))
      ret = true;
    fclose(file);
  }
  return ret;
}

std::list<FileStreamer::range> FileStreamer::bytesRange(const std::string &rangeValue, size_t size)
{
  std::list<range> ranges;
  if (rangeValue.size() < 6 || strncmp("bytes", rangeValue.c_str(), 5) != 0)
    return ranges;
  if (rangeValue[5] != ' ' && rangeValue[5] != '\t' && rangeValue[5] != '=')
    return ranges;

  std::vector<std::string> tokens;
  tokenize(rangeValue.c_str() + 6, ",", "", tokens, true);
  for (std::string& e : tokens)
  {
    range rg = { 0L, 0L };
    long int a = 0, b = LONG_MAX;
    sscanf(e.c_str(), "%li%li", &a, &b);
    if (a < 0)
    {
      rg.start = (size > (size_t)(0L - a) ? size + a : 0);
      rg.end = size - 1;
    }
    else
    {
      rg.start = (size_t)a;
      rg.end = (size_t)(b < 0 ? 0L - b : b);
      if (rg.end >= size)
        rg.end = size - 1;
    }
    if (rg.end >= rg.start && (ranges.empty() || rg.start > ranges.back().end))
      ranges.push_back(rg);
    else
    {
      ranges.clear();
      break;
    }
  }
  return ranges;
}

void FileStreamer::streamFile(handle * handle, const std::string& filePath, const std::string& contentType)
{
  size_t tb = 0; // count transfered bytes
  FILE * file = nullptr;
  WSRequestReply reply(*handle->broker);
  if (m_playbackCount.Load() >= FILESTREAMER_MAX_PB)
  {
    TraceResponseStatus(429);
    reply.PostReply(WS_STATUS_429_Too_Many_Requests);
    return;
  }
  if (!(file = fopen(filePath.c_str(), "rb")))
  {
    DBG(DBG_ERROR, "%s: opening file failed (%s)\n", __FUNCTION__, filePath.c_str());
    TraceResponseStatus(500);
    reply.PostReply(WS_STATUS_500_Internal_Server_Error);
    return;
  }
  DBG(DBG_DEBUG, "%s: open %p (%s)\n", __FUNCTION__, this, filePath.c_str());

  m_playbackCount.Add(1);
  TraceResponseStatus(200);
  reply.AddHeader(WS_HEADER_Content_Type, contentType);
  if (reply.BeginContent(WS_STATUS_200_OK, FILESTREAMER_CHUNK))
  {
    while (!IsAborted())
    {
      int r = reply.WriteFileStream(file);
      if (r > 0)
      {
        tb += r;
        continue;
      }
      if (r == 0)
        reply.CloseContent();
      break;
    }
  }
  DBG(DBG_DEBUG, "%s: close %p (%" PRIu64 ")\n", __FUNCTION__, this, tb);
  fclose(file);
  m_playbackCount.Sub(1);
}

static inline std::string makeETag(const char * path, time_t time)
{
  uint32_t h = 5381;
  while(*path)
    h = ((h << 5) + h) + *path++;
  uint32_t l = h;
  char * tt = (char*)&time;
  for (int i = 0; i < sizeof(time_t); ++i)
    l = ((l << 5) + l) + tt[i];
  char etag[24];
  snprintf(etag, sizeof(etag), "%08x-%08x", h, l);
  return etag;
}

void FileStreamer::streamFileRange(handle * handle, const std::string& filePath, const std::string& contentType,
                                   const std::string& rangeValue)
{
  FILE * file = nullptr;
  WSRequestReply reply(*handle->broker);
  if (m_playbackCount.Load() >= FILESTREAMER_MAX_PB)
  {
    TraceResponseStatus(429);
    reply.PostReply(WS_STATUS_429_Too_Many_Requests);
    return;
  }
  if (!(file = fopen(filePath.c_str(), "rb")))
  {
    DBG(DBG_WARN, "%s: opening file failed (%s)\n", __FUNCTION__, filePath.c_str());
    TraceResponseStatus(500);
    reply.PostReply(WS_STATUS_500_Internal_Server_Error);
    return;
  }
  DBG(DBG_DEBUG, "%s: open %p (%s) range (%s)\n", __FUNCTION__, this, filePath.c_str(), rangeValue.c_str());
  size_t fileSize = getFileLength(file);
  std::list<range> ranges = bytesRange(rangeValue, fileSize);
  if (ranges.empty())
  {
    fclose(file);
    DBG(DBG_WARN, "%s: bad seek %p (%s)\n", __FUNCTION__, this, rangeValue.c_str());
    TraceResponseStatus(416);
    reply.PostReply(WS_STATUS_416_Range_Not_Satisfiable);
    return;
  }

  m_playbackCount.Add(1);

  if (ranges.size() == 1)
  {
    reply.AddHeader(WS_HEADER_Content_Type, contentType);
    range& rg = ranges.front();
    if (fseek(file, rg.start, SEEK_SET) == 0)
    {
      WS_STATUS wss;
      size_t len = rg.end - rg.start + 1;
      if (len != fileSize)
      {
        TraceResponseStatus(206);
        wss = WS_STATUS_206_Partial_Content;
        BUILTIN_BUFFER str;
        std::string crg("bytes ");
        uint32_to_string(rg.start, &str);
        crg.append(str.data).push_back('-');
        uint32_to_string(rg.end, &str);
        crg.append(str.data).push_back('/');
        uint32_to_string(fileSize, &str);
        crg.append(str.data);
        reply.AddHeader(WS_HEADER_Content_Range, crg);
        uint32_to_string(len, &str);
        reply.AddHeader(WS_HEADER_Content_Length, str.data);
      }
      else
      {
        TraceResponseStatus(200);
        wss = WS_STATUS_200_OK;
        BUILTIN_BUFFER str;
        uint32_to_string(len, &str);
        reply.AddHeader(WS_HEADER_Content_Length, str.data);
      }

      if (reply.PostReply(wss))
      {
        size_t tb = 0; // count transfered bytes
        int buflen = FILESTREAMER_CHUNK;
        char * buf = new char [buflen];
        size_t r = 0;
        size_t chunk = (len > buflen ? buflen : len);
        while (!IsAborted() && chunk > 0 && (r = fread(buf, 1, chunk, file)) > 0)
        {
          if (!handle->broker->ReplyData(buf, r))
            break;
          tb += r;
          len -= r;
          if (len < buflen)
            chunk = len;
        }
        delete [] buf;

        if (len == 0)
          DBG(DBG_DEBUG, "%s: transfer range %p (%" PRIu64 ")\n", __FUNCTION__, this, tb);
      }
    }
  }
  else
  {
    // multipart content
    TraceResponseStatus(206);
    std::string boundary = makeETag(filePath.c_str(), time(nullptr));
    reply.AddHeader(WS_HEADER_Content_Type, std::string("multipart/byteranges; boundary=").append(boundary));
    if (reply.BeginContent(WS_STATUS_206_Partial_Content, FILESTREAMER_CHUNK))
    {
      std::list<range>::const_iterator it = ranges.begin();
      while (it != ranges.end())
      {
        if (fseek(file, it->start, SEEK_SET) != 0)
          break;
        std::string crg;
        crg.reserve(127);
        crg.append("--").append(boundary).append(WS_CRLF);
        crg.append(ws_header_to_str(WS_HEADER_Content_Type))
            .append(": ").append(contentType).append(WS_CRLF);
        crg.append(ws_header_to_str(WS_HEADER_Content_Range))
            .append(": ").append("bytes ");
        BUILTIN_BUFFER str;
        uint32_to_string(it->start, &str);
        crg.append(str.data).push_back('-');
        uint32_to_string(it->end, &str);
        crg.append(str.data).push_back('/');
        uint32_to_string(fileSize, &str);
        crg.append(str.data);
        crg.append(WS_CRLF WS_CRLF);

        if (!reply.WriteData(crg.c_str(), crg.size()))
          break;

        size_t len = it->end - it->start + 1;
        size_t tb = 0; // count transfered bytes
        int buflen = FILESTREAMER_CHUNK;
        char * buf = new char [buflen];
        size_t r = 0;
        size_t chunk = (len > buflen ? buflen : len);
        while (!IsAborted() && chunk > 0 && (r = fread(buf, 1, chunk, file)) > 0)
        {
          if (!reply.WriteData(buf, r))
            break;
          tb += r;
          len -= r;
          if (len < buflen)
            chunk = len;
        }
        delete [] buf;

        if (len > 0 || !reply.WriteData(WS_CRLF, WS_CRLF_LEN))
          break;
        DBG(DBG_DEBUG, "%s: transfer range %p (%" PRIu64 ")\n", __FUNCTION__, this, tb);
        ++it;
      }
      if (it != ranges.end())
        reply.Abort();
      else
      {
        std::string crg;
        crg.reserve(boundary.size() + 2 + WS_CRLF_LEN);
        crg.append("--").append(boundary).append(WS_CRLF);
        (void)reply.WriteData(crg.c_str(), crg.size());
        reply.CloseContent();
      }
    }
  }

  m_playbackCount.Sub(1);

  DBG(DBG_DEBUG, "%s: close %p\n", __FUNCTION__, this);
  fclose(file);
}
