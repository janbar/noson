/*
 *      Copyright (C) 2014-2018 Jean-Luc Barriere
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

#include "musicservices.h"
#include "private/builtin.h"
#include "private/debug.h"
#include "private/cppdef.h"
#include "private/tinyxml2.h"
#include "private/xmldict.h"
#include "private/wsrequest.h"
#include "private/wsresponse.h"
#include "private/jsonparser.h"
#include "private/os/threads/mutex.h"

#define USER_AGENT "Linux UPnP/1.0 Sonos/36.4-41270 (ACR_noson)"

using namespace NSROOT;

const std::string MusicServices::Name("MusicServices");
const std::string MusicServices::ControlURL("/MusicServices/Control");
const std::string MusicServices::EventURL("/MusicServices/Event");
const std::string MusicServices::SCPDURL("/xml/MusicServices1.xml");

SMService::SMService(const std::string& agent, const ElementList& vars)
: m_agent(agent)
, m_vars(vars)
{
  m_type = ServiceType(GetId());
  m_account = SMAccountPtr(new SMAccount(m_type));
  m_desc.assign("");
}

SMService::SMService(const std::string& agent, const ElementList& vars, const std::string& serialNum)
: m_agent(agent)
, m_vars(vars)
{
  m_type = ServiceType(GetId());
  m_account = SMAccountPtr(new SMAccount(m_type, serialNum));
  m_desc.assign("");
}

SMServicePtr SMService::Clone(const std::string& serialNum) const
{
  return SMServicePtr(new SMService(m_agent, m_vars, serialNum));
}

const std::string& SMService::GetId() const
{
  return m_vars.GetValue("Id");
}

const std::string& SMService::GetName() const
{
  return m_vars.GetValue("Name");
}

const std::string& SMService::GetVersion() const
{
  return m_vars.GetValue("Version");
}

const std::string& SMService::GetUri() const
{
  return m_vars.GetValue("Uri");
}

const std::string& SMService::GetSecureUri() const
{
  return m_vars.GetValue("SecureUri");
}

const std::string& SMService::GetContainerType() const
{
  return m_vars.GetValue("ContainerType");
}

const std::string& SMService::GetCapabilities() const
{
  return m_vars.GetValue("Capabilities");
}

ElementPtr SMService::GetPolicy() const
{
  ElementList::const_iterator it = m_vars.FindKey("Policy");
  if (it != m_vars.end())
    return (*it);
  return ElementPtr();
}

ElementPtr SMService::GetStrings() const
{
  ElementList::const_iterator it = m_vars.FindKey("Strings");
  if (it != m_vars.end())
    return (*it);
  return ElementPtr();
}

ElementPtr SMService::GetManifest() const
{
  ElementList::const_iterator it = m_vars.FindKey("Manifest");
  if (it != m_vars.end())
    return (*it);
  return ElementPtr();
}

ElementPtr SMService::GetPresentationMap() const
{
  ElementList::const_iterator it = m_vars.FindKey("PresentationMap");
  if (it != m_vars.end())
    return (*it);
  return ElementPtr();
}

std::string SMService::ServiceType(const std::string& id)
{
  int num = 0;
  if (string_to_int32(id.c_str(), &num) == 0)
    num = num * 256 + 7;
  return std::to_string(num);
}

const std::string& SMService::GetServiceType() const
{
  return m_type;
}

const std::string& SMService::GetServiceDesc() const
{
  if (m_desc.empty())
  {
    m_desc.assign("SA_RINCON").append(m_type).append("_");
    const std::string& auth = GetPolicy()->GetAttribut("Auth");
    if (auth == "UserId")
    {
      m_desc.append(m_account->GetCredentials().username);
    }
    else if (auth == "DeviceLink")
    {
      //@FIXME: failed for service 'Bandcamp'
      m_desc.append("X_#Svc").append(m_type).append("-0-Token");
    }
    else if (auth == "AppLink")
    {
      m_desc.append("X_#Svc").append(m_type).append("-0-Token");
    }
  }
  return m_desc;
}

SMAccountPtr SMService::GetAccount() const
{
  return m_account;
}

const std::string& SMService::GetAgent() const
{
  return m_agent;
}

bool SMService::CheckManifest()
{
  if (!GetManifest())
  {
    if (m_searchCategories.empty())
    {
      // see https://musicpartners.sonos.com/node/530
      // Enables the ability for users to search content
      uint32_t capabilities = 0;
      string_to_uint32(GetCapabilities().c_str(), &capabilities);
      if ((capabilities & 0x1) == 0x1)
      {
        DBG(DBG_WARN, "%s: load default search categories for service %s\n", __FUNCTION__, GetName().c_str());
        // add default search categories
        m_searchCategories.push_back(ElementPtr(new Element("tracks", "track")));
        m_searchCategories.push_back(ElementPtr(new Element("albums", "album")));
        m_searchCategories.push_back(ElementPtr(new Element("artists", "artist")));
        m_searchCategories.push_back(ElementPtr(new Element("playlists", "playlist")));
      }
    }
    return true;
  }

  // request the manifest
  URIParser _uri(GetManifest()->GetAttribut("Uri"));
  WSRequest request(_uri);
  request.SetUserAgent(GetAgent());
  WSResponse* response = new WSResponse(request);
  switch (response->GetStatusCode())
  {
  // allow the redirection
  case 301:
  case 302:
  {
    WSRequest redir(response->Redirection());
    delete response;
    response = new WSResponse(redir);
  }
  break;
  default:
    break;
  }
  if (response->IsSuccessful())
  {
    // Parse content response
    const JSON::Document json(*response);
    const JSON::Node& root = json.GetRoot();
    if (json.IsValid() && root.IsObject())
    {
      const JSON::Node& map = root.GetObjectValue("presentationMap");
      if (map.IsObject())
      {
        const JSON::Node& ver = map.GetObjectValue("version");
        if (ver.IsInt())
        {
          const JSON::Node& uri = map.GetObjectValue("uri");
          if (uri.IsString())
          {
            loadPresentationMap(uri.GetStringValue(), ver.GetIntValue());
            delete response;
            return true;
          }
        }
      }
    }
  }
  delete response;
  return false;
}

bool SMService::loadPresentationMap(const std::string& uri, int version)
{
  // is current version ?
  if (GetPresentationMap())
  {
    int32_t _ver = 0;
    string_to_int32(GetPresentationMap()->GetAttribut("Version").c_str(), &_ver);
    if (_ver == version)
    {
      DBG(DBG_DEBUG, "%s: version %d is up to date\n", __FUNCTION__, _ver);
      return true;
    }
  }

  DBG(DBG_INFO, "%s: load presentation map %d for service %s\n", __FUNCTION__, version, GetName().c_str());
  // load presentation map from given uri
  URIParser _uri(uri);
  WSRequest request(_uri);
  request.SetUserAgent(GetAgent());
  WSResponse* response = new WSResponse(request);
  switch (response->GetStatusCode())
  {
  // allow the redirection
  case 301:
  case 302:
  {
    WSRequest redir(response->Redirection());
    delete response;
    response = new WSResponse(redir);
  }
  break;
  default:
    break;
  }
  if (response->IsSuccessful())
  {
    // receive content data
    size_t len = 0, l = 0;
    std::string data;
    char buffer[4096];
    while ((l = response->ReadContent(buffer, sizeof(buffer))))
    {
      data.append(buffer, l);
      len += l;
    }
    delete response;
    response = nullptr;

    if (!parsePresentationMap(data))
      return false;

    // refresh presentation header
    ElementPtr mapPtr(new Element("PresentationMap"));
    mapPtr->SetAttribut("Uri", uri);
    BUILTIN_BUFFER buf;
    int32_to_string(version, &buf);
    mapPtr->SetAttribut("Version", buf.data);
    ElementList::const_iterator it = m_vars.FindKey(mapPtr->GetKey());
    if (it != m_vars.end())
      m_vars.erase(it);
    m_vars.push_back(mapPtr);
    return true;
  }
  else
  {
    DBG(DBG_ERROR, "%s: the presentation map is invalid\n", __FUNCTION__);
    delete response;
    m_presentation.clear();
    m_searchCategories.clear();
  }
  return false;
}

bool SMService::parsePresentationMap(const std::string &xml)
{
  tinyxml2::XMLDocument rootdoc;
  // Parse xml content
  if (rootdoc.Parse(xml.c_str(), xml.size()) != tinyxml2::XML_SUCCESS)
  {
    DBG(DBG_ERROR, "%s: parse xml failed\n", __FUNCTION__);
    return false;
  }
  tinyxml2::XMLElement* elem; // an element
  // Check for response: Presentation
  if (!(elem = rootdoc.RootElement()) || !XMLNS::NameEqual(elem->Name(), "Presentation"))
  {
    DBG(DBG_ERROR, "%s: invalid or not supported content\n", __FUNCTION__);
    tinyxml2::XMLPrinter out;
    rootdoc.Accept(&out);
    DBG(DBG_ERROR, "%s\n", out.CStr());
    return false;
  }
  m_presentation.clear();
  m_searchCategories.clear();
  elem = elem->FirstChildElement("PresentationMap");
  while (elem)
  {
    unsigned uid = 0; // unique item id
    tinyxml2::XMLElement* child; // a child of elem
    const char* type = elem->Attribute("type");
    if (type)
    {
      if (strncmp(type, "DisplayType", 11) == 0)
      {
      }
      else if (strncmp(type, "Search", 6) == 0 && (child = elem->FirstChildElement("Match")))
      {
        child = child->FirstChildElement("SearchCategories");
        while (child)
        {
          ElementPtr search(new Element("Search"));
          // set attribute StringId if any
          const char* stringId = child->Attribute("stringId");
          // accept category for 'CatalogSearch' or nil
          if (!stringId || strlen(stringId) == 0 || strncmp(stringId, "LibrarySearch", 13) == 0)
          {
            if (stringId)
              search->SetAttribut("stringId", stringId);
            // build the list of category for this search categories
            ElementList list;
            tinyxml2::XMLElement* categ = child->FirstChildElement();
            while (categ && categ->Attribute("id") && categ->Attribute("mappedId"))
            {
              // could be Category or CustomCategory
              ElementPtr item(new Element(categ->Name(), std::to_string(++uid)));
              item->SetAttribut("id", categ->Attribute("id"));
              item->SetAttribut("mappedId", categ->Attribute("mappedId"));
              list.push_back(item);
              // also fill list of search categories
              m_searchCategories.push_back(ElementPtr(new Element(categ->Attribute("id"), categ->Attribute("mappedId"))));
              categ = categ->NextSiblingElement(NULL);
            }
            m_presentation.push_back(std::make_pair(search, list));
          }
          child = child->NextSiblingElement(NULL);
        }
      }
      else if (strncmp(type, "BrowseIconSizeMap", 17) == 0 && (child = elem->FirstChildElement("Match")))
      {
        child = child->FirstChildElement("browseIconSizeMap");
        if (child)
        {
          ElementPtr name(new Element(child->Name()));
          // build the list of size entry
          ElementList list;
          tinyxml2::XMLElement* entry = child->FirstChildElement("sizeEntry");
          while (entry && entry->Attribute("size") && entry->Attribute("substitution"))
          {
            ElementPtr item(new Element(entry->Name(), std::to_string(++uid)));
            item->SetAttribut("size", entry->Attribute("size"));
            item->SetAttribut("substitution", entry->Attribute("substitution"));
            list.push_back(item);
            entry = entry->NextSiblingElement(NULL);
          }
          m_presentation.push_back(std::make_pair(name, list));
        }
      }
      else if (strncmp(type, "NowPlayingRatings", 17) == 0)
      {
      }
    }
    elem = elem->NextSiblingElement(NULL);
  }
  // Storage for presentation:
  // Element: key=Search, attr={stringId}
  // values : key=Category, attr={id="stations", mappedId="search:station"}, value=#ordered#
  //
  // Element: key=browseIconSizeMap, attr={}
  // values : key=sizeEntry, attr={size="0", substitution="_legacy.svg"}, value=#ordered#
  //
  return true;
}

///////////////////////////////////////////////////////////////////////////////
////
//// MusicServices
////

MusicServices::MusicServices(const std::string& serviceHost, unsigned servicePort)
: Service(serviceHost, servicePort)
, m_version("")
{
}

bool MusicServices::GetSessionId(const std::string& serviceId, const std::string& username, ElementList& vars)
{
  ElementList args;
  args.push_back(ElementPtr(new Element("ServiceId", serviceId)));
  args.push_back(ElementPtr(new Element("Username", username)));
  vars = Request("GetSessionId", args);
  if (!vars.empty() && vars[0]->compare("GetSessionIdResponse") == 0)
    return true;
  return false;
}

SMServiceList MusicServices::GetAvailableServices()
{
  // hold version's lock until return
  Locked<std::string>::pointer versionPtr = m_version.Get();
  SMServiceList list;
  // load services
  ElementList vars;
  std::vector<ElementList> data;
  if (!ListAvailableServices(vars) || !ParseAvailableServices(vars, data))
    DBG(DBG_ERROR, "%s: query services failed\n", __FUNCTION__);
  else
  {
    // store new value of version
    versionPtr->assign(vars.GetValue("AvailableServiceListVersion"));
    std::string agent;
    // configure a valid user-agent
    agent.assign(USER_AGENT);

    // Fill the list of services.
    for (std::vector<ElementList>::const_iterator it = data.begin(); it != data.end(); ++it)
    {
        list.push_back(SMServicePtr(new SMService(agent, *it)));
    }
  }
  DBG(DBG_DEBUG, "%s: version (%s)\n", __FUNCTION__, versionPtr->c_str());
  return list;
}

bool MusicServices::ListAvailableServices(ElementList& vars)
{
  ElementList args;
  vars = Request("ListAvailableServices", args);
  if (!vars.empty() && vars[0]->compare("ListAvailableServicesResponse") == 0)
    return true;
  return false;
}

bool MusicServices::ParseAvailableServices(const ElementList& vars, std::vector<ElementList>& data) const
{
  const std::string& xml = vars.GetValue("AvailableServiceDescriptorList");
  tinyxml2::XMLDocument rootdoc;
  // Parse xml content
  if (rootdoc.Parse(xml.c_str(), xml.size()) != tinyxml2::XML_SUCCESS)
  {
    DBG(DBG_ERROR, "%s: parse xml failed\n", __FUNCTION__);
    return false;
  }
  const tinyxml2::XMLElement* elem; // an element
  // Check for response: Services
  if (!(elem = rootdoc.RootElement()) || strncmp(elem->Name(), "Services", 8) != 0)
  {
    DBG(DBG_ERROR, "%s: invalid or not supported content\n", __FUNCTION__);
    tinyxml2::XMLPrinter out;
    rootdoc.Accept(&out);
    DBG(DBG_ERROR, "%s\n", out.CStr());
    return false;
  }
  data.clear();
  elem = elem->FirstChildElement();
  while (elem)
  {
    unsigned uid = 0; // unique item id
    const tinyxml2::XMLAttribute* attr = elem->FirstAttribute();
    ElementList service;
    /* Service:
     *   Id=integer
     *   Name=string
     *   Version=integer
     *   Uri=string
     *   SecureUri=String
     *   ContainerType=string
     *   Capabilities=integer
     *   Policy {}
     *   Manifest {}
     */
    while (attr)
    {
      service.push_back(ElementPtr(new Element(attr->Name(), attr->Value())));
      attr = attr->Next();
    }
    DBG(DBG_DEBUG, "%s: service '%s' (%s)\n", __FUNCTION__, service.GetValue("Name").c_str(), service.GetValue("Id").c_str());
    // browse childs
    const tinyxml2::XMLElement* child = elem->FirstChildElement();
    while (child)
    {
      /* Policy:
       *   Auth=string
       *   PoolInterval=integer
       */
      if (XMLNS::NameEqual(child->Name(), "Policy"))
      {
        const tinyxml2::XMLAttribute* cattr = child->FirstAttribute();
        ElementPtr policyPtr(new Element(child->Name(), std::to_string(++uid)));
        while (cattr)
        {
          policyPtr->SetAttribut(cattr->Name(), cattr->Value());
          cattr = cattr->Next();
        }
        service.push_back(policyPtr);
      }
      /* Manifest:
       *   Version=integer
       *   Uri=string
       */
      if (XMLNS::NameEqual(child->Name(), "Manifest"))
      {
        const tinyxml2::XMLAttribute* cattr = child->FirstAttribute();
        ElementPtr manifestPtr(new Element(child->Name(), std::to_string(++uid)));
        while (cattr)
        {
          manifestPtr->SetAttribut(cattr->Name(), cattr->Value());
          cattr = cattr->Next();
        }
        service.push_back(manifestPtr);
      }
      child = child->NextSiblingElement(nullptr);
    }
    data.push_back(service);
    elem = elem->NextSiblingElement(nullptr);
  }
  return true;
}
