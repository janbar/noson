/*
 *      Copyright (C) 2014-2019 Jean-Luc Barriere
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

#ifndef SONOSSYSTEM_H
#define	SONOSSYSTEM_H

#include "local_config.h"
#include "sonoszone.h"
#include "sonosplayer.h"
#include "eventhandler.h"
#include "subscriptionpool.h"
#include "musicservices.h"
#include "alarm.h"

#include <string>
#include <map>

#define SONOS_LISTENER_PORT 1400

namespace NSROOT
{
  namespace OS
  {
    class CMutex;
    class CEvent;
  }

  class ZoneGroupTopology;
  class DeviceProperties;
  class ContentDirectory;
  class AlarmClock;

  class System : private EventSubscriber
  {
    friend class Player;
  public:
    System(void* CBHandle, EventCB eventCB);
    ~System();

    static void Debug(int level);

    bool IsListening() { return m_eventHandler.IsRunning(); }

    bool Discover();
    bool Discover(const std::string& url);
    const std::string& GetHost() const { return m_deviceHost; }
    unsigned GetPort() const { return m_devicePort; }

    unsigned char LastEvents();

    void RenewSubscriptions();

    ZoneList GetZoneList() const;

    ZonePlayerList GetZonePlayerList() const;

    PlayerPtr GetPlayer(const ZonePtr& zone, void* CBHandle = 0, EventCB eventCB = 0);

    PlayerPtr GetPlayer(const ZonePlayerPtr& zonePlayer, void* CBHandle = 0, EventCB eventCB = 0);

    bool IsConnected() const;

    // Implements EventSubscriber
    virtual void HandleEventMessage(EventMessagePtr msg);

    // Device properties
    const std::string& GetHouseholdID() const { return m_householdID; }

    const std::string& GetSerialNumber() const { return m_serialNumber; }

    const std::string& GetSoftwareVersion() const { return m_softwareVersion; }

    // Alarm clock
    AlarmList GetAlarmList() const;

    bool CreateAlarm(Alarm& alarm);

    bool UpdateAlarm(Alarm& alarm);

    bool DestroyAlarm(const std::string& id);

    // Content directory
    ContentProperty GetContentProperty();

    bool RefreshShareIndex();

    std::string GetObjectIDFromUriMetadata(const DigitalItemPtr& uriMetadata);

    bool DestroySavedQueue(const std::string& SQObjectID);

    bool AddURIToFavorites(const DigitalItemPtr& item, const std::string& description, const std::string& artURI);

    bool DestroyFavorite(const std::string& FVObjectID);

    static bool ExtractObjectFromFavorite(const DigitalItemPtr& favorite, DigitalItemPtr& item);
    static bool CanQueueItem(const DigitalItemPtr& item);

    // Music services
    SMServiceList GetEnabledServices();

    SMServiceList GetAvailableServices();

    bool GetSessionId(const std::string& serviceId, const std::string& username, ElementList& vars);

    SMServicePtr GetServiceForMedia(const std::string& mediaUri);

    static bool IsItemFromService(const DigitalItemPtr& item);

    /**
     * Request logo for a given music service and placement
     * @param service The music service
     * @param placement small|medium|large|x-large|square:x-small|square:small|square|square:x-large
     * @return uri string for the requested logo
     */
    static std::string GetLogoForService(const SMServicePtr& service, const std::string& placement);

    /**
     * Register auth data for a third part service
     * @param type The service type
     * @param sn The serial of account
     * @param key The key or password required to authenticate
     * @param token The current token for AppLink policy
     * @param username The user name for Login policy
     */
    static void AddServiceOAuth(const std::string& type, const std::string& sn, const std::string& key, const std::string& token, const std::string& username);

    /**
     * Remove auth data of a registered service
     * @param type The service type
     * @param sn The serial of account
     */
    static void DeleteServiceOAuth(const std::string& type, const std::string& sn);

    /**
     * Check the PulseAudio feature.
     * @return true if the feature is enabled
     */
    static bool HavePulseAudio();

    // Customized request broker
    void RegisterRequestBroker(RequestBrokerPtr rb);
    void UnregisterRequestBroker(const std::string& name);
    RequestBrokerPtr GetRequestBroker(const std::string& name);
    const std::string& GetSystemLocalUri() { return m_systemLocalUri; }

  private:
    mutable OS::CMutex* m_mutex;
    OS::CEvent* m_cbzgt;
    bool m_connected;
    unsigned m_subId;
    EventHandler m_eventHandler;
    std::string m_deviceHost;
    unsigned m_devicePort;
    void* m_CBHandle;                       // callback handle
    EventCB m_eventCB;                      // callback on event
    Locked<bool> m_eventSignaled;           // cleared by calling LastEvents()
    Locked<unsigned char> m_eventMask;      // cleared by calling LastEvents()
    // Services API
    ZoneGroupTopology*  m_groupTopology;
    DeviceProperties*   m_deviceProperties;
    AlarmClock*         m_alarmClock;
    ContentDirectory*   m_contentDirectory;
    MusicServices*      m_musicServices;

    typedef std::map<std::string, PlayerPtr> PlayerMap;
    Locked<PlayerMap> m_players;

    // Service subscriptions
    SubscriptionPoolPtr m_subscriptionPool;
    
    // About this controler
    std::string m_systemLocalUri;

    // About the connected device
    std::string m_householdID;
    std::string m_serialNumber;
    std::string m_softwareVersion;

    SMServiceList m_smservices;

    static bool DeviceMatches(const char * serverString);
    static bool FindDeviceDescriptions(std::vector<std::string>& urls);
    void RevokePlayers();

    static void CB_ZGTopology(void* handle);
    static void CB_AlarmClock(void* handle);
    static void CB_ContentDirectory(void* handle);

    static bool LoadMSLogo(ElementList& logos);
  };
}


#endif	/* SONOSSYSTEM_H */

