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

#ifndef EVENTHANDLER_H
#define	EVENTHANDLER_H

#include "local_config.h"
#include "sharedptr.h"
#include "requestbroker.h"
#include "locked.h"

#include <string>
#include <vector>

#define EVENTHANDLER_STARTED        "STARTED"   // Message on started
#define EVENTHANDLER_STOPPED        "STOPPED"   // Message on stopped
#define EVENTHANDLER_FAILED         "FAILED"    // Message on failed
#define EVENTHANDLER_LIMIT_HOLD_MS  500         // Offload duration (ms)

namespace NSROOT
{

  typedef enum
  {
    EVENT_UNKNOWN = 0,
    EVENT_HANDLER_STATUS,         // On backend status change
    EVENT_HANDSHAKE_FAILURE,      // On accept connection failure
    EVENT_ACCESS_LOG,             // On reply completed with status
    EVENT_UPNP_PROPCHANGE,        // upnp:propchange
  } EVENT_t;

  struct EventMessage
  {
    EVENT_t                   event;
    std::vector<std::string>  subject;

    EventMessage()
    : event(EVENT_UNKNOWN)
    {}
  };

  typedef SHARED_PTR<const EventMessage> EventMessagePtr;

  class EventSubscriber
  {
  public:
    virtual ~EventSubscriber() {}
    virtual void HandleEventMessage(EventMessagePtr msg) = 0;
  };

  struct EventHandlerConf
  {
    unsigned    bindingPort         = 8080;
    unsigned    listenerQueueSize   = 50;
    unsigned    threadPoolSize      = 16;
    unsigned    threadKeepAliveMs   = 60000;  // 60 seconds
    unsigned    requestQueueSize    = 250;

    unsigned    clientTimeout       = 5000;   // 5 seconds
    bool        clientKeepAlive     = true;
  };

  class EventBroker;

  class EventHandlerThread
  {
    friend class EventHandler;
  public:
    EventHandlerThread(const EventHandlerConf& conf);
    virtual ~EventHandlerThread();
    std::string GetAddress() const { return m_listenerAddress; }
    unsigned GetPort() const { return m_conf.bindingPort; }
    unsigned GetClientTimeout() const { return m_conf.clientTimeout; }
    bool GetClientKeepAlive() const { return m_conf.clientKeepAlive; }
    virtual unsigned GetCapacity() = 0;
    virtual unsigned PendingRequest() = 0;
    virtual void EnqueueRequest(EventBroker *eb) = 0;
    virtual bool Start() = 0;
    virtual void Stop() = 0;
    virtual bool HasStarted() = 0;
    virtual unsigned CreateSubscription(EventSubscriber *sub) = 0;
    virtual bool SubscribeForEvent(unsigned subid, EVENT_t event) = 0;
    virtual void RevokeSubscription(unsigned subid) = 0;
    virtual void RevokeAllSubscriptions(EventSubscriber *sub) = 0;
    virtual void DispatchEvent(const EventMessagePtr& msg) = 0;

    /**
     * @brief Configure a callback to handle any other requests than supported by the event broker.
     * @param rb the pointer to the request broker instance or null
     */
    virtual void RegisterRequestBroker(RequestBrokerPtr rb) = 0;
    virtual void UnregisterRequestBroker(const std::string& name) = 0;
    virtual void UnregisterAllRequestBroker() = 0;
    virtual RequestBrokerPtr GetRequestBroker(const std::string& name) = 0;
    virtual std::vector<RequestBrokerPtr> AllRequestBroker() = 0;

  protected:
    EventHandlerConf m_conf;
    std::string m_listenerAddress;
  };

  typedef SHARED_PTR<EventHandlerThread> EventHandlerThreadPtr;

  class EventHandler
  {
  public:
    EventHandler();
    EventHandler(unsigned bindingPort);

    bool Start() { return m_imp ? m_imp->Start(): false; }
    void Stop() { if (m_imp) m_imp->Stop(); }
    bool IsRunning() { return m_imp ? m_imp->HasStarted() : false; }

    std::string GetAddress() const { return m_imp ? m_imp->GetAddress() : ""; }
    unsigned GetPort() const { return m_imp ? m_imp->GetPort(): 0; }
    unsigned GetClientTimeout() const { return m_imp ? m_imp->GetClientTimeout() : 0; }
    bool GetClientKeepAlive() const { return m_imp ? m_imp->GetClientKeepAlive() : false; }
    unsigned GetCapacity() const { return m_imp ? m_imp->GetCapacity() : 0; }
    unsigned PendingRequest() { return m_imp ? m_imp->PendingRequest() : 0; }
    void EnqueueRequest(EventBroker* eb) { if (m_imp) m_imp->EnqueueRequest(eb); }

    void RegisterRequestBroker(RequestBrokerPtr rb) { if (m_imp) m_imp->RegisterRequestBroker(rb); }
    void UnregisterRequestBroker(const std::string& name) { if (m_imp) m_imp->UnregisterRequestBroker(name); }
    RequestBrokerPtr GetRequestBroker(const std::string& name) { return m_imp ? m_imp->GetRequestBroker(name) : RequestBrokerPtr(); }
    std::vector<RequestBrokerPtr> AllRequestBroker() { return m_imp ? m_imp->AllRequestBroker() : std::vector<RequestBrokerPtr>(); }

    unsigned CreateSubscription(EventSubscriber *sub) { return m_imp ? m_imp->CreateSubscription(sub) : 0; }
    bool SubscribeForEvent(unsigned subid, EVENT_t event) { return m_imp ? m_imp->SubscribeForEvent(subid, event) : false; }
    void RevokeSubscription(unsigned subid) { if (m_imp) m_imp->RevokeSubscription(subid); }
    void RevokeAllSubscriptions(EventSubscriber *sub) { if (m_imp) m_imp->RevokeAllSubscriptions(sub); }

  private:
    EventHandlerThreadPtr m_imp;
  };

}

#endif	/* EVENTHANDLER_H */
