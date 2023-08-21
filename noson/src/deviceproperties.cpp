/*
 *      Copyright (C) 2014-2016 Jean-Luc Barriere
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

#include "deviceproperties.h"
#include "private/builtin.h"

using namespace NSROOT;

const std::string DeviceProperties::Name("DeviceProperties");
const std::string DeviceProperties::ControlURL("/DeviceProperties/Control");
const std::string DeviceProperties::EventURL("/DeviceProperties/Event");
const std::string DeviceProperties::SCPDURL("/xml/DeviceProperties1.xml");

DeviceProperties::DeviceProperties(const std::string& serviceHost, unsigned servicePort)
: Service(serviceHost, servicePort)
{
}

bool DeviceProperties::GetZoneInfo(ElementList& vars)
{
  ElementList args;
  vars = Request("GetZoneInfo", args);
  if (!vars.empty() && vars[0]->compare("GetZoneInfoResponse") == 0)
    return true;
  return false;
}

bool DeviceProperties::GetZoneAttributes(ElementList& vars)
{
  ElementList args;
  vars = Request("GetZoneAttributes", args);
  if (!vars.empty() && vars[0]->compare("GetZoneAttributesResponse") == 0)
    return true;
  return false;
}

bool DeviceProperties::GetHouseholdID(ElementList& vars)
{
  ElementList args;
  vars = Request("GetHouseholdID", args);
  if (!vars.empty() && vars[0]->compare("GetHouseholdIDResponse") == 0)
    return true;
  return false;
}

bool DeviceProperties::GetAutoplayRoomUUID(ElementList& vars)
{
  ElementList args;
  vars = Request("GetAutoplayRoomUUID", args);
  if (!vars.empty() && vars[0]->compare("GetAutoplayRoomUUIDResponse") == 0)
    return true;
  return false;
}

bool DeviceProperties::SetAutoplayRoomUUID(const std::string& roomuuid)
{
  ElementList args;
  args.push_back(SONOS::ElementPtr(new SONOS::Element("RoomUUID", roomuuid)));
  ElementList vars;
  vars = Request("SetAutoplayRoomUUID", args);
  if (!vars.empty() && vars[0]->compare("SetAutoplayRoomUUIDResponse") == 0)
    return true;
  return false;
}

bool DeviceProperties::SetLEDState(bool onoff)
{
  ElementList args;
  args.push_back(SONOS::ElementPtr(new SONOS::Element("DesiredLEDState", (onoff ? "On" : "Off"))));
  ElementList vars;
  vars = Request("SetLEDState", args);
  if (!vars.empty() && vars[0]->compare("SetLEDStateResponse") == 0)
    return true;
  return false;
}

bool DeviceProperties::GetAutoplayVolume(ElementList& vars)
{
  ElementList args;
  vars = Request("GetAutoplayVolume", args);
  if (!vars.empty() && vars[0]->compare("GetAutoplayVolumeResponse") == 0)
    return true;
  return false;
}

bool DeviceProperties::SetAutoplayVolume(uint8_t volume)
{
  ElementList args;
  args.push_back(SONOS::ElementPtr(new SONOS::Element("Volume", std::to_string(volume))));
  ElementList vars;
  vars = Request("SetAutoplayVolume", args);
  if (!vars.empty() && vars[0]->compare("SetAutoplayVolumeResponse") == 0)
    return true;
  return false;
}

bool DeviceProperties::GetUseAutoplayVolume(ElementList& vars)
{
  ElementList args;
  vars = Request("GetUseAutoplayVolume", args);
  if (!vars.empty() && vars[0]->compare("GetUseAutoplayVolumeResponse") == 0)
    return true;
  return false;
}

bool DeviceProperties::SetUseAutoplayVolume(uint8_t useVolume)
{
  ElementList args;
  args.push_back(SONOS::ElementPtr(new SONOS::Element("UseVolume", std::to_string(useVolume))));
  ElementList vars;
  vars = Request("SetUseAutoplayVolume", args);
  if (!vars.empty() && vars[0]->compare("SetUseAutoplayVolumeResponse") == 0)
    return true;
  return false;
}
