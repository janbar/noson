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

#include "imageservice.h"
#include "private/debug.h"
#include "data/datareader.h"
#include "filepicreader.h"
#include "private/uriencoder.h"
#include "private/wsstatic.h"
#include "private/wsrequestbroker.h"
#include "private/wsrequestreply.h"

#include <map>
#include <cstring>

/* Important: It MUST match with the static declaration from datareader.cpp */
#define IMAGESERVICE_FAVICON  "favicon.ico"
#define RESOURCE_FILEPICTURE  "filePicture"
#define IMAGESERVICE_CHUNK    16384

using namespace NSROOT;

ImageService::ImageService()
: SONOS::RequestBroker()
, m_resources()
{
  // initialize the static resource for favicon
  {
    ResourcePtr ptr(new Resource());
    ptr->uri = "/" IMAGESERVICE_FAVICON;
    ptr->title = "favicon";
    ptr->sourcePath = IMAGESERVICE_FAVICON;
    ptr->delegate = DataReader::Instance();
    m_resources.insert(std::make_pair(ptr->uri, ptr));
  }
  // register the picture extractor for local media file
  RegisterResource(RESOURCE_FILEPICTURE, "The cover art extractor", "track", FilePicReader::Instance());
}

bool ImageService::HandleRequest(handle * handle)
{
  if (!IsAborted())
  {
    const std::string& requrl = handle->broker->GetRequestPath();
    if (requrl.compare(0, strlen(IMAGESERVICE_URI), IMAGESERVICE_URI) == 0 ||
            requrl == "/" IMAGESERVICE_FAVICON)
    {
      switch (handle->broker->GetRequestMethod())
      {
      case WS_METHOD_Get:
        ProcessGET(handle);
        return true;
      case WS_METHOD_Head:
        ProcessHEAD(handle);
        return true;
      default:
        return false; // unhandled method
      }
    }
  }
  return false;
}

RequestBroker::ResourcePtr ImageService::GetResource(const std::string& title)
{
  for (ResourceMap::iterator it = m_resources.begin(); it != m_resources.end(); ++it)
  {
    if (it->second->title == title)
      return it->second;
  }
  return ResourcePtr();
}

RequestBroker::ResourceList ImageService::GetResourceList()
{
  ResourceList list;
  for (ResourceMap::iterator it = m_resources.begin(); it != m_resources.end(); ++it)
    list.push_back(it->second);
  return list;
}

RequestBroker::ResourcePtr ImageService::RegisterResource(const std::string& title,
                                                          const std::string& description,
                                                          const std::string& path,
                                                          StreamReader * delegate)
{
  ResourcePtr ptr(new Resource());
  ptr->title = title;
  ptr->description = description;
  ptr->sourcePath = path;
  ptr->delegate = delegate;
  ptr->uri = std::string(IMAGESERVICE_URI);
  if (path.empty() || path.front() != '/')
    ptr->uri.append("/").append(path);
  else
    ptr->uri.append(path);
  m_resources.insert(std::make_pair(ptr->uri, ptr));
  return ptr;
}

void ImageService::UnregisterResource(const std::string& uri)
{
  (void)uri;
}

std::string ImageService::MakeFilePictureURI(const std::string& filePath)
{
  std::string pictureUri;
  // find the resource for extracting picture
  ResourcePtr res = GetResource(RESOURCE_FILEPICTURE);
  if (!res)
    return pictureUri;
  // encode the file path
  std::string pathParm(urlencode(filePath));
  // make the picture uri
  if (res->uri.find('?') != std::string::npos)
    pictureUri.assign(res->uri).append("&path=").append(pathParm).append("&type=3");
  else
    pictureUri.assign(res->uri).append("?path=").append(pathParm).append("&type=3");

  return pictureUri;
}

void ImageService::ProcessGET(handle * handle)
{
  WSRequestReply reply(*handle->broker);
  ResourceMap::const_iterator it = m_resources.find(handle->broker->GetRequestPath());
  if (it == m_resources.end())
  {
    TraceResponseStatus(400);
    reply.CloseReply(WS_STATUS_400_Bad_Request);
  }
  else if (!it->second || !it->second->delegate)
  {
    TraceResponseStatus(500);
    reply.CloseReply(WS_STATUS_500_Internal_Server_Error);
  }
  else
  {
    const RequestBroker::ResourcePtr& res = it->second;
    // build the delegate url: source path + params
    std::string dlurl(res->sourcePath);
    if (!handle->broker->GetURIParams().empty())
      dlurl.append("?").append(handle->broker->GetURIParams());
    StreamReader::STREAM * stream = res->delegate->OpenStream(dlurl);

    if (stream && stream->contentLength)
    {
      // override content type with stream type
      const char * contentType = stream->contentType != nullptr ? stream->contentType : res->contentType.c_str();
      TraceResponseStatus(200);
      reply.AddHeader(WS_HEADER_Content_Type, contentType);
      if (reply.BeginContent(WS_STATUS_200_OK, IMAGESERVICE_CHUNK))
      {
        int r = 0;
        unsigned len = stream->contentLength;
        while ((r = res->delegate->ReadStream(stream)) > 0)
        {
          if (!reply.WriteData(stream->data, stream->size))
            break;
          len -= r;
        }
        if (len == 0)
          reply.CloseContent();
      }
      res->delegate->CloseStream(stream);
    }
    else if (stream)
    {
      res->delegate->CloseStream(stream);
      TraceResponseStatus(404);
      reply.CloseReply(WS_STATUS_404_Not_Found);
    }
    else
    {
      TraceResponseStatus(500);
      reply.CloseReply(WS_STATUS_500_Internal_Server_Error);
    }
  }
}

void ImageService::ProcessHEAD(handle * handle)
{
  WSRequestReply reply(*handle->broker);
  ResourceMap::const_iterator it = m_resources.find(handle->broker->GetRequestPath());
  if (it == m_resources.end())
  {
    TraceResponseStatus(400);
    reply.CloseReply(WS_STATUS_400_Bad_Request);
  }
  else if (!it->second || !it->second->delegate)
  {
    TraceResponseStatus(500);
    reply.CloseReply(WS_STATUS_500_Internal_Server_Error);
  }
  else
  {
    const RequestBroker::ResourcePtr& res = it->second;
    // build the delegate url: source path + params
    std::string dlurl(res->sourcePath);
    if (!handle->broker->GetURIParams().empty())
      dlurl.append("?").append(handle->broker->GetURIParams());
    StreamReader::STREAM * stream = res->delegate->OpenStream(dlurl);

    if (stream && stream->contentLength)
    {
      // override content type with stream type
      const char * contentType = stream->contentType != nullptr ? stream->contentType : res->contentType.c_str();
      TraceResponseStatus(200);
      reply.AddHeader(WS_HEADER_Content_Type, contentType);
      reply.AddHeader(WS_HEADER_Content_Length, stream->contentLength);
      reply.CloseReply(WS_STATUS_200_OK);
      res->delegate->CloseStream(stream);
    }
    else if (stream)
    {
      res->delegate->CloseStream(stream);
      TraceResponseStatus(404);
      reply.CloseReply(WS_STATUS_404_Not_Found);
    }
    else
    {
      TraceResponseStatus(500);
      reply.CloseReply(WS_STATUS_500_Internal_Server_Error);
    }
  }
}
