/*
 *      Copyright (C) 2015 Jean-Luc Barriere
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

#include "sonossystem.h"
#include "zonegrouptopology.h"
#include "digitalitem.h"
#include "didlparser.h"
#include "private/socket.h"
#include "private/wsresponse.h"
#include "private/os/threads/timeout.h"
#include "private/debug.h"
#include "private/builtin.h"
#include "private/uriparser.h"
#include "private/tinyxml2.h"
#include "private/os/threads/mutex.h"
#include "private/os/threads/event.h"
#include "private/cppdef.h"
#include "private/xmldict.h"

#include <cstdio> // for sscanf

#define CB_TIMEOUT    5000
#define PATH_TOPOLOGY "/status/topology"
#define URI_MSLOGO    "http://update-services.sonos.com/services/mslogo.xml"

using namespace SONOS;

System::System(void* CBHandle, EventCB eventCB)
: m_mutex(new OS::CMutex)
, m_cbzgt(new OS::CEvent)
, m_eventHandler(SONOS_LISTENER_PORT)
, m_subId(0)
, m_groupTopology(0)
, m_CBHandle(CBHandle)
, m_eventCB(eventCB)
{
  m_connectedZone.player.reset();
  m_connectedZone.zone.reset();

  m_subId = m_eventHandler.CreateSubscription(this);
  m_eventHandler.SubscribeForEvent(m_subId, EVENT_HANDLER_STATUS);
  m_eventHandler.SubscribeForEvent(m_subId, EVENT_UNKNOWN);
  if (!m_eventHandler.Start())
    DBG(DBG_ERROR, "%s: starting event handler failed\n", __FUNCTION__);
}

System::~System()
{
  m_mutex->Lock();
  SAFE_DELETE(m_groupTopology);
  SAFE_DELETE(m_cbzgt);
  SAFE_DELETE(m_mutex);
}

bool System::Discover()
{
  std::string url;
  if (!FindDeviceDescription(url))
    return false;
  URIParser uri(url);
  if (!uri.Scheme() || !uri.Host() || !uri.Port())
    return false;

  OS::CLockGuard lock(*m_mutex);
  // subscribe to ZoneGroupTopology events
  SAFE_DELETE(m_groupTopology);
  m_ZGTSubscription = Subscription(uri.Host(), uri.Port(), ZoneGroupTopology::EventURL, m_eventHandler.GetPort(), SUBSCRIPTION_TIMEOUT);
  m_groupTopology = new ZoneGroupTopology(uri.Host(), uri.Port(), m_eventHandler, m_ZGTSubscription, this, CBZGTopology);
  m_ZGTSubscription.Start();
  // Wait event notification
  if (m_cbzgt->Wait(CB_TIMEOUT))
    return true;
  DBG(DBG_WARN, "%s: notification wasn't received after timeout: fall back on manual call\n", __FUNCTION__);
  return m_groupTopology->GetZoneGroupState();
}

void System::RenewSubscriptions()
{
  m_ZGTSubscription.AskRenewal();
}

ZoneList System::GetZoneList() const
{
  OS::CLockGuard lock(*m_mutex);
  ZoneList list;
  if (m_groupTopology)
  {
    Locked<ZoneList>::pointer zones = m_groupTopology->GetZoneList().Get();
    for (ZoneList::const_iterator it = zones->begin(); it != zones->end(); ++it)
      if (it->second->GetCoordinator())
        list.insert(std::make_pair(it->first, it->second));
  }
  return list;
}

ZonePlayerList System::GetZonePlayerList() const
{
  OS::CLockGuard lock(*m_mutex);
  if (m_groupTopology)
    return *(m_groupTopology->GetZonePlayerList().Get());
  return ZonePlayerList();
}

bool System::ConnectZone(const ZonePtr& zone, void* CBHandle, EventCB eventCB)
{
  OS::CLockGuard lock(*m_mutex);
  // Check listener
  if (!m_eventHandler.IsRunning() && !m_eventHandler.Start())
    return false;
  // Check requirements
  if (!zone)
    return false;
  DBG(DBG_DEBUG, "%s: connect zone '%s'\n", __FUNCTION__, zone->GetZoneName().c_str());
  PlayerPtr player(new Player(zone, m_eventHandler, CBHandle, eventCB));
  if (player->IsValid())
  {
    m_connectedZone.player = player;
    m_connectedZone.zone = zone;
    return true;
  }
  return false;
}

bool System::ConnectZone(const ZonePlayerPtr& zonePlayer, void* CBHandle, EventCB eventCB)
{
  OS::CLockGuard lock(*m_mutex);
  // Check listener
  if (!m_eventHandler.IsRunning() && !m_eventHandler.Start())
    return false;
  // Check requirements
  if (!m_groupTopology || !zonePlayer)
    return false;
  Locked<ZoneList>::pointer zones = m_groupTopology->GetZoneList().Get();
  ZoneList::const_iterator zit = zones->find(zonePlayer->GetAttribut("group"));
  if (zit == zones->end())
    return false;
  return ConnectZone(zit->second, CBHandle, eventCB);
}

bool System::IsConnected() const
{
  OS::CLockGuard lock(*m_mutex);
  if (m_connectedZone.zone)
    return true;
  return false;
}

void System::HandleEventMessage(EventMessagePtr msg)
{
  if (msg->subject.size() > 0)
  {
    if (msg->subject[0] == "GET" && msg->subject[1] == "/stop")
      m_eventHandler.Stop();
  }
}

bool System::ExtractObjectFromFavorite(const DigitalItemPtr& favorite, DigitalItemPtr& item)
{
  const std::string& str = favorite->GetValue(DIDL_QNAME_RINC "resMD");
  if (str.empty())
    return false;
  DIDLParser didl(str.c_str());
  if (didl.IsValid() && !didl.GetItems().empty())
  {
    item.swap(didl.GetItems()[0]);
    item->SetProperty(favorite->GetProperty("res"));
    return true;
  }
  DigitalItemPtr ptr(new DigitalItem(DigitalItem::Type_item, DigitalItem::SubType_unknown));
  ptr->SetProperty(favorite->GetProperty("res"));
  ptr->SetProperty(favorite->GetProperty(DIDL_QNAME_DC "title"));
  ptr->SetProperty(favorite->GetProperty(DIDL_QNAME_UPNP "albumArtURI"));
  ptr->SetObjectID("-1");
  ptr->SetParentID("-1");
  item.swap(ptr);
  return true;
}

bool System::CanQueueItem(const DigitalItemPtr& item)
{
  if (item)
  {
    // check parent
    const std::string& parent = item->GetParentID();
    if (    parent.compare(0, 2, "A:") == 0 ||
            parent.compare(0, 3, "SQ:") == 0)
      return true;

    // check protocol from tag <res>
    URIParser parser(item->GetValue("res"));
    if (parser.Scheme())
    {
      if (strcmp(ProtocolTable[Protocol_file], parser.Scheme()) == 0)
        return true;
      if (strcmp(ProtocolTable[Protocol_xFileCifs], parser.Scheme()) == 0)
        return true;
      if (strcmp(ProtocolTable[Protocol_xSonosHttp], parser.Scheme()) == 0)
        return true;
      if (strcmp(ProtocolTable[Protocol_xSonosSpotify], parser.Scheme()) == 0)
        return true;
      if (strcmp(ProtocolTable[Protocol_xSonosMms], parser.Scheme()) == 0)
        return true;
      if (strcmp(ProtocolTable[Protocol_xSonosApiRTRecent], parser.Scheme()) == 0)
        return true;
      if (strcmp(ProtocolTable[Protocol_xRinconCpcontainer], parser.Scheme()) == 0)
        return true;
    }
  }
  return false;
}

bool System::IsItemFromService(const DigitalItemPtr& item)
{
  if (!item)
    return false;
  const std::string& desc = item->GetValue("desc");
  if (desc.empty() || desc == ServiceDescTable[ServiceDesc_default])
    return false;
  return true;
}

std::string System::MakeItemIdFromMediaUri(const std::string& mediaUri)
{
  URIParser parser(mediaUri);
  if (!parser.Scheme() || !parser.Path())
  {
    DBG(DBG_ERROR, "%s: invalid uri (%s)\n", __FUNCTION__, mediaUri.c_str());
    return "";
  }
  // check is service item
  std::string itemId;
  const char* p = strchr(parser.Path(), '?');
  if (p && strstr(p, "sid="))
  {
    std::string tmp(parser.Path(), p - parser.Path());
    std::string id = tmp.substr(0, tmp.find_last_of("."));
    // schema x-sonosapi-rtrecent for podcast
    if (strcmp(ProtocolTable[Protocol_xSonosApiRTRecent], parser.Scheme()) == 0)
      itemId.append("F00032020").append(id);
    // other scheme for a track
    else
      itemId.append("00032020").append(id);
  }
  else if (strcmp(ProtocolTable[Protocol_xRinconPlaylist], parser.Scheme()) == 0)
  {
    if (!parser.Fragment())
    {
      DBG(DBG_ERROR, "%s: invalid uri (%s)\n", __FUNCTION__, mediaUri.c_str());
      return "";
    }
    itemId.assign(parser.Fragment());
  }
  else if (strcmp(ProtocolTable[Protocol_file], parser.Scheme()) == 0 && strcmp("jffs/settings/savedqueues.rsq", parser.Path()) == 0)
  {
    if (!parser.Fragment())
    {
      DBG(DBG_ERROR, "%s: invalid uri (%s)\n", __FUNCTION__, mediaUri.c_str());
      return "";
    }
    itemId.assign("SQ:").append(parser.Fragment());
  }
  else
    itemId.assign(mediaUri);

  return itemId;
}

std::string System::GetLogoForService(const SMServicePtr& service, const std::string& placement)
{
  static Locked<unsigned> cc(0); ///< zero to refill cache
  static ElementList logos; ///< cached container for logos

  // hold count until return
  Locked<unsigned>::pointer ccPtr = cc.Get();

  // on first call we fill the container requesting sonos service
  if ((*ccPtr)++ == 0 && !LoadMSLogo(logos))
    DBG(DBG_ERROR, "%s: cache for service images cannot be filled", __FUNCTION__);

  const std::string& typeId = service->GetServiceType();
  for (ElementList::const_iterator it = logos.begin(); it != logos.end(); ++it)
    if ((*it)->GetKey() == typeId && (*it)->GetAttribut("placement") == placement)
      return (**it);
  return Element::Nil();
}

bool System::FindDeviceDescription(std::string& url)
{
#define MULTICAST_ADDR      "239.255.255.250"
#define MULTICAST_STRP      "1900"
#define MULTICAST_NUMP      1900
#define DISCOVER_TIMEOUT    5000
#define DISCOVER_ST         "urn:schemas-upnp-org:device:ZonePlayer:1"
#define HTTP_TOKEN_MAXSIZE  20

  static const char* msearch =
  "M-SEARCH * HTTP/1.1\r\n"
  "HOST: " MULTICAST_ADDR ":" MULTICAST_STRP "\r\n"
  "MAN: \"ssdp:discover\"\r\n"
  "MX: 1\r\n"
  "ST: " DISCOVER_ST "\r\n"
  "\r\n";

  static struct timeval socket_timeout = { 0, 500000 };

  bool ret = false;
  UdpSocket sock;
  sock.SetAddress(SOCKET_AF_INET4, MULTICAST_ADDR, MULTICAST_NUMP);
  sock.SetMulticastTTL(4);

  OS::CTimeout timeout(DISCOVER_TIMEOUT);
  while (!ret && timeout.TimeLeft() > 0)
  {
    sock.SendData(msearch, strlen(msearch));
    sock.SetTimeout(socket_timeout);
    std::string strread;
    size_t len = 0;
    unsigned _context = 0;
    while (!ret && WSResponse::ReadHeaderLine(&sock, "\r\n", strread, &len))
    {
      const char* line = strread.c_str();
      if (len == 15 && memcmp(line, "HTTP", 4) == 0)
      {
        int status = 0;
        if (1 == sscanf(line, "%*s %d", &status) && status == 200)
        {
          DBG(DBG_DEBUG, "%s: starting new context\n", __FUNCTION__);
          _context = 1; // new valid context
        }
        else
          _context = 0; // reset context
      }
      else if (_context)
      {
        /*
         * Each header field consists of a name followed by a colon (":") and the
         * field value. Field names are case-insensitive. The field value MAY be
         * preceded by any amount of LWS, though a single SP is preferred.
         */
        const char* val = NULL;
        char token[21];
        int token_len = 0;
        int value_len = 0;
        if ((val = strchr(line, ':')))
        {
          int p;
          if ((token_len = val - line) > HTTP_TOKEN_MAXSIZE)
            token_len = HTTP_TOKEN_MAXSIZE;
          for (p = 0; p < token_len; ++p)
            token[p] = toupper(line[p]);
          token[token_len] = 0;
          while ((value_len = len - (val - line)) && *(++val) == ' ');
        }

        if (token_len)
        {
          switch (token_len)
          {
            case 2:
              if (val && memcmp(token, "ST", token_len) == 0)
              {
                if (memcmp(val, DISCOVER_ST, value_len) == 0)
                {
                  DBG(DBG_DEBUG, "%s: search target matches (%s)\n", __FUNCTION__, val);
                  _context |= 0x2;
                }
                else
                {
                  DBG(DBG_DEBUG, "%s: search target not matches (%s)\n", __FUNCTION__, val);
                  _context = 0; // reset context
                }
              }
              break;
            case 6:
              if (val && memcmp(token, "SERVER", token_len) == 0)
              {
                DBG(DBG_DEBUG, "%s: server string found (%s)\n", __FUNCTION__, val);
                _context |= 0x4;
              }
              break;
            case 8:
              if (val && memcmp(token, "LOCATION", token_len) == 0)
              {
                DBG(DBG_DEBUG, "%s: location url found (%s)\n", __FUNCTION__, val);
                _context |= 0x8;
                url.assign(val);
              }
              break;
            default:
              break;
          }
        }
        else if (len == 0)
        {
          DBG(DBG_DEBUG, "%s: reseting context\n", __FUNCTION__);
          _context = 0; // reset context
        }
      }
      if (_context == 0xF)
        ret = true;
    }
  }
  return ret;
}

void System::CBZGTopology(void* handle)
{
  if (handle)
  {
    System* _handle = static_cast<System*>(handle);
    _handle->m_cbzgt->Broadcast();
    if (_handle->m_eventCB)
      (_handle->m_eventCB)(_handle->m_CBHandle);
  }
}

bool System::LoadMSLogo(ElementList& logos)
{
  WSRequest request(URIParser(URI_MSLOGO));
  WSResponse response(request);
  if (!response.IsSuccessful())
    return false;

  size_t len = 0, l = 0;
  std::string data;
  char buffer[4000];
  while ((l = response.ReadContent(buffer, sizeof(buffer))))
  {
    data.append(buffer, l);
    len += l;
  }

  tinyxml2::XMLDocument rootdoc;
  // Parse xml content
  if (rootdoc.Parse(data.c_str(), len) != tinyxml2::XML_SUCCESS)
  {
    DBG(DBG_ERROR, "%s: parse xml failed\n", __FUNCTION__);
    return false;
  }
  const tinyxml2::XMLElement* elem; // an element
  // Check for response: Services
  if (!(elem = rootdoc.RootElement()) || !XMLNS::NameEqual(elem->Name(), "images")
          || !(elem = elem->FirstChildElement("sized")))
  {
    DBG(DBG_ERROR, "%s: invalid or not supported content\n", __FUNCTION__);
    tinyxml2::XMLPrinter out;
    rootdoc.Accept(&out);
    DBG(DBG_ERROR, "%s\n", out.CStr());
    return false;
  }
  logos.clear();
  elem = elem->FirstChildElement("service");
  while (elem)
  {
    const tinyxml2::XMLElement* felem;
    const char* typeId = elem->Attribute("id");
    if (typeId)
    {
      felem = elem->FirstChildElement("image");
      while (felem)
      {
        const char* p = felem->Attribute("placement");
        if (p && felem->GetText())
        {
          ElementPtr logo(new Element(typeId, felem->GetText()));
          logo->SetAttribut("placement", p);
          logos.push_back(logo);
        }
        felem = felem->NextSiblingElement("image");
      }
    }
    elem = elem->NextSiblingElement("service");
  }
  return true;
}
