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

#include "pulsestreamer.h"
#include "pacontrol.h"
#include "pasource.h"
#include "audiostream.h"
#include "flacencoder.h"
#include "requestbroker.h"
#include "imageservice.h"
#include "data/datareader.h"
#include "private/debug.h"
#include "private/socket.h"
#include "private/os/threads/timeout.h"

#include <cstring>

/* Important: It MUST match with the static declaration from datareader.cpp */
#define PULSESTREAMER_ICON      "/pulseaudio.png"
#define PULSESTREAMER_CONTENT   "audio/flac"
#define PULSESTREAMER_DESC      "Audio stream from %s"
#define PULSESTREAMER_TIMEOUT   10000
#define PULSESTREAMER_MAX_PB    3
#define PULSESTREAMER_CHUNK     16384
#define PULSESTREAMER_TM_MUTE   1000
#define PA_SINK_NAME            "noson"
#define PA_CLIENT_NAME          PA_SINK_NAME

using namespace NSROOT;

PulseStreamer::PulseStreamer(RequestBroker * imageService /*= nullptr*/)
: RequestBroker()
, m_resources()
, m_sinkIndex(0)
, m_playbackCount(0)
{
  // delegate image download to imageService
  ResourcePtr img(nullptr);
  if (imageService)
    img = imageService->RegisterResource(PULSESTREAMER_CNAME,
                                         "Icon for " PULSESTREAMER_CNAME,
                                         PULSESTREAMER_ICON,
                                         DataReader::Instance());

  // declare the static resource
  ResourcePtr ptr = ResourcePtr(new Resource());
  ptr->uri = PULSESTREAMER_URI;
  ptr->title = PULSESTREAMER_CNAME;
  ptr->description = PULSESTREAMER_DESC;
  ptr->contentType = PULSESTREAMER_CONTENT;
  if (img)
    ptr->iconUri.assign(img->uri).append("?id=" LIBVERSION);
  m_resources.push_back(ptr);
}

bool PulseStreamer::Initialize()
{
  if (initialize_pulse(1) == 0)
    return true;
  return false;
}

bool PulseStreamer::HandleRequest(handle * handle)
{
  if (!IsAborted())
  {
    const std::string& requrl = RequestBroker::GetRequestURI(handle);
    if (requrl.compare(0, strlen(PULSESTREAMER_URI), PULSESTREAMER_URI) == 0)
    {
      switch (RequestBroker::GetRequestMethod(handle))
      {
      case RequestBroker::Method_GET:
        streamSink(handle);
        return true;
      case RequestBroker::Method_HEAD:
      {
        std::string resp;
        resp.assign(RequestBroker::MakeResponseHeader(RequestBroker::Status_OK))
            .append("Content-Type: audio/flac\r\n")
            .append("\r\n");
        RequestBroker::Reply(handle, resp.c_str(), resp.length());
        return true;
      }
      default:
        return false; // unhandled method
      }
    }
  }
  return false;
}

RequestBroker::ResourcePtr PulseStreamer::GetResource(const std::string& title)
{
  (void)title;
  return m_resources.front();
}

RequestBroker::ResourceList PulseStreamer::GetResourceList()
{
  ResourceList list;
  for (ResourceList::iterator it = m_resources.begin(); it != m_resources.end(); ++it)
    list.push_back((*it));
  return list;
}

RequestBroker::ResourcePtr PulseStreamer::RegisterResource(const std::string& title,
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

void PulseStreamer::UnregisterResource(const std::string& uri)
{
  (void)uri;
}

std::string PulseStreamer::GetPASink()
{
  std::string deviceName;
  PAControl::SinkList sinks;
  PAControl pacontrol(PA_CLIENT_NAME);

  bool cont = true;
  for (;;)
  {
    // get sink list
    if (pacontrol.connect())
    {
      pacontrol.getSinkList(&sinks);
      pacontrol.disconnect();
    }
    else
    {
      DBG(DBG_ERROR, "%s: failed to connect to pulse\n", __FUNCTION__);
      break;
    }
    // search the sink in list
    for (PAControl::Sink& ad : sinks)
    {
      if (ad.name == PA_SINK_NAME)
      {
        DBG(DBG_DEBUG, "%s: Found device %d: %s\n", __FUNCTION__, ad.index, ad.monitorSourceName.c_str());
        deviceName = ad.monitorSourceName;
        m_sinkIndex.Store(ad.ownerModule); // own the module
        break;
      }
    }
    if (!deviceName.empty() || !cont)
      break;
    // no sink exist so create it
    DBG(DBG_DEBUG, "%s: create sink (%s)\n", __FUNCTION__, PA_SINK_NAME);
    if (pacontrol.connect())
    {
      m_sinkIndex.Store(pacontrol.newSink(PA_SINK_NAME, PA_SINK_NAME));
      pacontrol.disconnect();
      cont = false;
    }
    else
      break;
  }
  return deviceName;
}

void PulseStreamer::FreePASink()
{
  // Lock count
  // and check if an other playback is running before delete the sink
  LockedNumber<int>::pointer p = m_playbackCount.Get();
  if (*p == 1 && m_sinkIndex.Load())
  {
    PAControl pacontrol(PA_CLIENT_NAME);
    if (pacontrol.connect())
    {
      DBG(DBG_DEBUG, "%s: delete sink (%s)\n", __FUNCTION__, PA_SINK_NAME);
      pacontrol.deleteSink(m_sinkIndex.Load());
      pacontrol.disconnect();
    }
    m_sinkIndex.Store(0);
  }
}

void PulseStreamer::streamSink(handle * handle)
{
  m_playbackCount.Add(1);

  std::string deviceName = GetPASink();

  if (deviceName.empty())
  {
    DBG(DBG_WARN, "%s: no sink available\n", __FUNCTION__);
    Reply503(handle);
  }
  else if (m_playbackCount.Load() > PULSESTREAMER_MAX_PB)
    Reply429(handle);
  else
  {
    AudioSource * src = new PASource(PA_CLIENT_NAME, deviceName);
    AudioEncoder * enc = new FLACEncoder();
    AudioStream ai(*src, *enc);
    // the source is muted for a short time to limit output rate on startup
    OS::Timeout muted(PULSESTREAMER_TM_MUTE);
    src->mute(true);
    ai.start();

    std::string resp;
    resp.assign(RequestBroker::MakeResponseHeader(RequestBroker::Status_OK))
        .append("Content-Type: audio/flac\r\n")
        .append("Transfer-Encoding: chunked\r\n")
        .append("\r\n");

    if (RequestBroker::Reply(handle, resp.c_str(), resp.length()))
    {
      char * buf = new char [PULSESTREAMER_CHUNK + 16];
      int r = 0;
      while (!IsAborted() && (r = ai.read(buf + 7, PULSESTREAMER_CHUNK, PULSESTREAMER_TIMEOUT)) > 0)
      {
        char str[8];
        snprintf(str, sizeof(str), "%05x\r\n", (unsigned)r & 0xfffff);
        memcpy(buf, str, 7);
        memcpy(buf + r + 7, "\r\n", 2);
        if (!RequestBroker::Reply(handle, buf, r + 7 + 2))
          break;
        // disable source mute after delay
        if (src->muted() && !muted.TimeLeft())
          src->mute(false);
      }
      delete [] buf;
      if (r == 0)
        RequestBroker::Reply(handle, "0\r\n\r\n", 5);
    }

    ai.stop();
    delete enc;
    delete src;
  }

  FreePASink();
  m_playbackCount.Sub(1);
}

void PulseStreamer::Reply503(handle * handle)
{
  std::string resp;
  resp.assign(RequestBroker::MakeResponseHeader(RequestBroker::Status_Service_Unavailable))
      .append("\r\n");
  RequestBroker::Reply(handle, resp.c_str(), resp.length());
}

void PulseStreamer::Reply400(handle * handle)
{
  std::string resp;
  resp.append(RequestBroker::MakeResponseHeader(RequestBroker::Status_Bad_Request))
      .append("\r\n");
  RequestBroker::Reply(handle, resp.c_str(), resp.length());
}

void PulseStreamer::Reply429(handle * handle)
{
  std::string resp;
  resp.append(RequestBroker::MakeResponseHeader(RequestBroker::Status_Too_Many_Requests))
      .append("\r\n");
  RequestBroker::Reply(handle, resp.c_str(), resp.length());
}
