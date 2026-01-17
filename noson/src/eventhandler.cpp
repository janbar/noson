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

#include "eventhandler.h"
#include "private/upnpnotificationbroker.h"
#include "private/mainpagebroker.h"
#include "private/os/threads/threadpool.h"
#include "private/socket.h"
#include "private/cppdef.h"
#include "private/builtin.h"
#include "private/debug.h"
#include "private/eventbroker.h"
#include "private/wsresponse.h"

#include <vector>
#include <map>
#include <list>

#define EVENTHANDLER_LOOP_ADDRESS     "127.0.0.1"   // IPv4 localhost
#define EVENTHANDLER_THREAD_KEEPALIVE 60000         // 60 sec

using namespace NSROOT;

///////////////////////////////////////////////////////////////////////////////
////
//// EventHandlerThread
////

EventHandlerThread::EventHandlerThread(unsigned bindingPort)
: m_port(bindingPort)
{
}

EventHandlerThread::~EventHandlerThread()
{
}

///////////////////////////////////////////////////////////////////////////////
////
//// SubscriptionHandlerThread
////

namespace NSROOT
{
  class SubscriptionHandlerThread : private OS::Thread
  {
  public:
    SubscriptionHandlerThread(EventSubscriber *handle, unsigned subid);
    virtual ~SubscriptionHandlerThread();
    EventSubscriber *GetHandle() { return m_handle; }
    bool IsRunning() { return OS::Thread::IsRunning(); }
    void PostMessage(const EventMessagePtr& msg);

  private:
    EventSubscriber *m_handle;
    unsigned m_subId;
    mutable OS::Mutex m_mutex;
    OS::Event m_queueContent;
    std::list<EventMessagePtr> m_msgQueue;

    bool Start();
    void Stop();
    void *Process();
  };
}

SubscriptionHandlerThread::SubscriptionHandlerThread(EventSubscriber *handle, unsigned subid)
: OS::Thread()
, m_handle(handle)
, m_subId(subid)
, m_mutex()
, m_queueContent()
, m_msgQueue()
{
  if (m_handle && Start())
    DBG(DBG_DEBUG, "%s: subscription is started (%p:%u)\n", __FUNCTION__, m_handle, m_subId);
  else
    DBG(DBG_ERROR, "%s: subscription failed (%p:%u)\n", __FUNCTION__, m_handle, m_subId);
}

SubscriptionHandlerThread::~SubscriptionHandlerThread()
{
  Stop();
  m_handle = NULL;
}

bool SubscriptionHandlerThread::Start()
{
  if (OS::Thread::IsRunning())
    return true;
  return OS::Thread::StartThread();
}

void SubscriptionHandlerThread::Stop()
{
  if (OS::Thread::IsRunning())
  {
    DBG(DBG_DEBUG, "%s: subscription thread (%p:%u)\n", __FUNCTION__, m_handle, m_subId);
    // Set stopping. don't wait as we need to signal the thread first
    OS::Thread::StopThread(false);
    m_queueContent.Signal();
    // Wait for thread to stop
    OS::Thread::StopThread(true);
    DBG(DBG_DEBUG, "%s: subscription thread (%p:%u) stopped\n", __FUNCTION__, m_handle, m_subId);
  }
}

void SubscriptionHandlerThread::PostMessage(const EventMessagePtr& msg)
{
  // Critical section
  OS::LockGuard lock(m_mutex);
  m_msgQueue.push_back(msg);
  m_queueContent.Signal();
}

void *SubscriptionHandlerThread::Process()
{
  while (!IsStopped())
  {
    while (!m_msgQueue.empty() && !IsStopped())
    {
      // Critical section
      m_mutex.Lock();
      EventMessagePtr msg = m_msgQueue.front();
      m_msgQueue.pop_front();
      m_mutex.Unlock();
      // Do work
      m_handle->HandleEventMessage(msg);
    }
    // The tread is woken up by m_queueContent.Signal();
    m_queueContent.Wait();
  }
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
////
//// BasicEventHandler
////

namespace NSROOT
{
  class BasicEventHandler : public EventHandlerThread, private OS::Thread
  {
  public:
    BasicEventHandler(unsigned bindingPort);
    virtual ~BasicEventHandler();
    // Implements EventHandlerThread
    virtual bool Start();
    virtual void Stop();
    virtual bool HasStarted();
    virtual void RegisterRequestBroker(RequestBrokerPtr rb);
    virtual void UnregisterRequestBroker(const std::string& name);
    virtual void UnregisterAllRequestBroker();
    virtual RequestBrokerPtr GetRequestBroker(const std::string& name);
    virtual std::vector<RequestBrokerPtr> AllRequestBroker();
    virtual unsigned CreateSubscription(EventSubscriber *sub);
    virtual bool SubscribeForEvent(unsigned subid, EVENT_t event);
    virtual void RevokeSubscription(unsigned subid);
    virtual void RevokeAllSubscriptions(EventSubscriber *sub);
    virtual void DispatchEvent(const EventMessagePtr& msg);

  private:
    OS::Mutex m_mutex;
    OS::ThreadPool m_threadpool;
    TcpServerSocket *m_socket;

    // About subscriptions
    typedef std::map<EVENT_t, std::list<unsigned> > subscriptionsByEvent_t;
    subscriptionsByEvent_t m_subscriptionsByEvent;
    typedef std::map<unsigned, SubscriptionHandlerThread*> subscriptions_t;
    subscriptions_t m_subscriptions;

    virtual void* Process(void);
    void AnnounceStatus(const char *status);

    typedef std::map<std::string, RequestBrokerPtr> RBList;
    Locked<RBList> m_RBList;
  };
}

BasicEventHandler::BasicEventHandler(unsigned bindingPort)
: EventHandlerThread(bindingPort), OS::Thread()
, m_socket(new TcpServerSocket)
, m_RBList(RBList())
{
  m_listenerAddress = EVENTHANDLER_LOOP_ADDRESS;
  m_threadpool.SetMaxSize(EVENTHANDLER_THREADS);
  m_threadpool.SetKeepAlive(EVENTHANDLER_THREAD_KEEPALIVE);
  m_threadpool.Start();
}

BasicEventHandler::~BasicEventHandler()
{
  Stop();
  UnregisterAllRequestBroker();
  m_threadpool.Suspend();
  {
    OS::LockGuard lock(m_mutex);
    for (subscriptions_t::iterator it = m_subscriptions.begin(); it != m_subscriptions.end(); ++it)
      delete it->second;
    m_subscriptions.clear();
    m_subscriptionsByEvent.clear();
  }
  SAFE_DELETE(m_socket);
}

bool BasicEventHandler::Start()
{
  if (OS::Thread::IsRunning())
    return true;
  return OS::Thread::StartThread();
}

void BasicEventHandler::Stop()
{
  if (OS::Thread::IsRunning())
  {
    DBG(DBG_DEBUG, "%s: event handler thread (%p)\n", __FUNCTION__, this);
    OS::Thread::StopThread(true);
    DBG(DBG_DEBUG, "%s: event handler thread (%p) stopped\n", __FUNCTION__, this);
  }
}

bool BasicEventHandler::HasStarted()
{
  return OS::Thread::IsRunning();
}

void BasicEventHandler::RegisterRequestBroker(RequestBrokerPtr rb)
{
  if (!rb)
    return;
  DBG(DBG_DEBUG, "%s: register (%s)\n", __FUNCTION__, rb->CommonName());
  m_RBList.Get()->insert(std::make_pair(rb->CommonName(), rb));
}

void BasicEventHandler::UnregisterRequestBroker(const std::string &name)
{
  DBG(DBG_DEBUG, "%s: unregister (%s)\n", __FUNCTION__, name.c_str());
  Locked<RBList>::pointer p = m_RBList.Get();
  RBList::const_iterator it = p->find(name);
  if (it != p->end())
  {
    it->second->Abort();
    p->erase(it);
  }
}

void BasicEventHandler::UnregisterAllRequestBroker()
{
  Locked<RBList>::pointer p = m_RBList.Get();
  for (RBList::iterator it = p->begin(); it != p->end(); ++it)
  {
    DBG(DBG_DEBUG, "%s: unregister (%s)\n", __FUNCTION__, it->second->CommonName());
    it->second->Abort();
  }
  p->clear();
}

RequestBrokerPtr BasicEventHandler::GetRequestBroker(const std::string &name)
{
  Locked<RBList>::pointer p = m_RBList.Get();
  RBList::const_iterator it = p->find(name);
  if (it != p->end())
    return it->second;
  return RequestBrokerPtr();
}

std::vector<RequestBrokerPtr> BasicEventHandler::AllRequestBroker()
{
  std::vector<RequestBrokerPtr> vect;
  Locked<RBList>::pointer p = m_RBList.Get();
  vect.reserve(p->size());
  for (RBList::iterator it = p->begin(); it != p->end(); ++it)
    vect.push_back(it->second);
  return vect;
}

unsigned BasicEventHandler::CreateSubscription(EventSubscriber* sub)
{
  unsigned id = 0;
  OS::LockGuard lock(m_mutex);
  subscriptions_t::const_reverse_iterator it = m_subscriptions.rbegin();
  if (it != m_subscriptions.rend())
    id = it->first;
  SubscriptionHandlerThread *handler = new SubscriptionHandlerThread(sub, ++id);
  if (handler->IsRunning())
  {
    m_subscriptions.insert(std::make_pair(id, handler));
    return id;
  }
  // Handler didn't start
  delete handler;
  return 0;
}

bool BasicEventHandler::SubscribeForEvent(unsigned subid, EVENT_t event)
{
  OS::LockGuard lock(m_mutex);
  // Only for registered subscriber
  subscriptions_t::const_iterator it1 = m_subscriptions.find(subid);
  if (it1 == m_subscriptions.end())
    return false;
  std::list<unsigned>::const_iterator it2 = m_subscriptionsByEvent[event].begin();
  while (it2 != m_subscriptionsByEvent[event].end())
  {
    if (*it2 == subid)
      return true;
    ++it2;
  }
  m_subscriptionsByEvent[event].push_back(subid);
  return true;
}

void BasicEventHandler::RevokeSubscription(unsigned subid)
{
  OS::LockGuard lock(m_mutex);
  subscriptions_t::iterator it;
  it = m_subscriptions.find(subid);
  if (it != m_subscriptions.end())
  {
    delete it->second;
    m_subscriptions.erase(it);
  }
}

void BasicEventHandler::RevokeAllSubscriptions(EventSubscriber *sub)
{
  OS::LockGuard lock(m_mutex);
  std::vector<subscriptions_t::iterator> its;
  for (subscriptions_t::iterator it = m_subscriptions.begin(); it != m_subscriptions.end(); ++it)
  {
    if (sub == it->second->GetHandle())
      its.push_back(it);
  }
  for (std::vector<subscriptions_t::iterator>::const_iterator it = its.begin(); it != its.end(); ++it)
  {
    delete (*it)->second;
    m_subscriptions.erase(*it);
  }
}

void BasicEventHandler::DispatchEvent(const EventMessagePtr& msg)
{
  OS::LockGuard lock(m_mutex);
  std::vector<std::list<unsigned>::iterator> revoked;
  std::list<unsigned>::iterator it1 = m_subscriptionsByEvent[msg->event].begin();
  while (it1 != m_subscriptionsByEvent[msg->event].end())
  {
    subscriptions_t::const_iterator it2 = m_subscriptions.find(*it1);
    if (it2 != m_subscriptions.end())
      it2->second->PostMessage(msg);
    else
      revoked.push_back(it1);
    ++it1;
  }
  std::vector<std::list<unsigned>::iterator>::const_iterator itr;
  for (itr = revoked.begin(); itr != revoked.end(); ++itr)
    m_subscriptionsByEvent[msg->event].erase(*itr);
}

void *BasicEventHandler::Process()
{
  bool bound = false;
  if (m_socket->Create(SOCKET_AF_INET4))
  {
    for (int retry = 0; retry < 10; ++retry)
    {
      DBG(DBG_INFO, "%s: bind port %u\n", __FUNCTION__, m_port);
      if ((bound = m_socket->Bind(m_port)))
        break;
      ++m_port;
    }
  }
  if (bound)
  {
    DBG(DBG_INFO, "%s: start listening\n", __FUNCTION__);
    bound = m_socket->ListenConnection();
  }
  if (bound)
  {
    AnnounceStatus(EVENTHANDLER_STARTED);
    while (!OS::Thread::IsStopped())
    {
      SHARED_PTR<TcpSocket> sockPtr(new TcpSocket);
      TcpServerSocket::AcceptStatus r = m_socket->AcceptConnection(*sockPtr, 1);
      if (r == TcpServerSocket::ACCEPT_SUCCESS)
      {
        DBG(DBG_DEBUG, "%s: accepting new connection\n", __FUNCTION__);
        EventBroker* eb = new EventBroker(this, sockPtr);
        m_threadpool.Enqueue(eb);
        continue;
      }
      if (r == TcpServerSocket::ACCEPT_FAILURE)
      {
        DBG(DBG_WARN, "%s: accept failed (%d)\n", __FUNCTION__, m_socket->GetErrNo());
        continue;
      }
      if (r == TcpServerSocket::ACCEPT_TIMEOUT)
      {
        continue;
      }
      AnnounceStatus(EVENTHANDLER_FAILED);
      break;
    }
    AnnounceStatus(EVENTHANDLER_STOPPED);
  }
  else
  {
    DBG(DBG_DEBUG, "%s: creating listener failed (%d)\n", __FUNCTION__, m_socket->GetErrNo());
    AnnounceStatus(EVENTHANDLER_FAILED);
  }
  // Close connection
  m_socket->Close();
  return NULL;
}

void BasicEventHandler::AnnounceStatus(const char *status)
{
  DBG(DBG_DEBUG, "%s: (%p) %s\n", __FUNCTION__, this, status);
  EventMessage* msg = new EventMessage();
  msg->event = EVENT_HANDLER_STATUS;
  msg->subject.push_back(status);
  msg->subject.push_back(m_listenerAddress);
  msg->subject.push_back(std::to_string((uint16_t)m_port));
  DispatchEvent(EventMessagePtr(msg));
}

///////////////////////////////////////////////////////////////////////////////
////
//// EventHandler
////

EventHandler::EventHandler()
: m_imp()
{
}

EventHandler::EventHandler(unsigned bindingPort)
: m_imp()
{
  // Choose implementation
  m_imp = EventHandlerThreadPtr(new BasicEventHandler(bindingPort));
  // Enable the main page
  RegisterRequestBroker(RequestBrokerPtr(new MainPageBroker()));
  // Load the request broker to process UPNP notifications
  RegisterRequestBroker(RequestBrokerPtr(new UPNPNotificationBroker()));
}
