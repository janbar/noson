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
#include "eventbroker.h"
#include "requestbroker.h"
#include "private/os/threads/threadpool.h"
#include "private/socket.h"
#include "private/builtin.h"
#include "private/debug.h"
#include "private/wsresponse.h"

#include <vector>
#include <list>

#define EVENTHANDLER_LOOP_ADDRESS     "127.0.0.1"   // IPv4 localhost

using namespace NSROOT;

///////////////////////////////////////////////////////////////////////////////
////
//// EventHandlerThread
////

EventHandlerThread::EventHandlerThread(const EventHandlerConf& conf)
: m_conf(conf)
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
    bool IsRunning() { return OS::Thread::is_running(); }
    void PostMessage(const EventMessagePtr& msg);

  private:
    EventSubscriber *m_handle;
    unsigned m_subId;
    mutable OS::Mutex m_mutex;
    OS::Event m_queueContent;
    std::list<EventMessagePtr> m_msgQueue;

    bool Start();
    void Stop();
    void *process() override;
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
  m_handle = nullptr;
}

bool SubscriptionHandlerThread::Start()
{
  if (OS::Thread::is_running())
    return true;
  return OS::Thread::start_thread();
}

void SubscriptionHandlerThread::Stop()
{
  if (OS::Thread::is_running())
  {
    DBG(DBG_DEBUG, "%s: subscription thread (%p:%u)\n", __FUNCTION__, m_handle, m_subId);
    // Set stopping. don't wait as we need to signal the thread first
    OS::Thread::stop_thread(false);
    m_queueContent.notify_one();
    // Wait for thread to stop
    OS::Thread::stop_thread(true);
    DBG(DBG_DEBUG, "%s: subscription thread (%p:%u) stopped\n", __FUNCTION__, m_handle, m_subId);
  }
}

void SubscriptionHandlerThread::PostMessage(const EventMessagePtr& msg)
{
  // Critical section
  OS::LockGuard lock(m_mutex);
  m_msgQueue.push_back(msg);
  m_queueContent.notify_one();
}

void *SubscriptionHandlerThread::process()
{
  while (!is_stopped())
  {
    while (!m_msgQueue.empty() && !is_stopped())
    {
      // Critical section
      m_mutex.lock();
      EventMessagePtr msg = m_msgQueue.front();
      m_msgQueue.pop_front();
      m_mutex.unlock();
      // Do work
      m_handle->HandleEventMessage(msg);
    }
    // The tread is woken up by m_queueContent.Signal();
    m_queueContent.wait();
  }
  return nullptr;
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
    BasicEventHandler(const EventHandlerConf& conf);
    ~BasicEventHandler() override;
    // Implements EventHandlerThread
    bool Start() override;
    void Stop() override;
    bool HasStarted() override;
    unsigned GetCapacity() override;
    unsigned PendingRequest() override;
    void EnqueueRequest(EventBroker *eb) override;
    void RegisterRequestBroker(RequestBrokerPtr rb) override;
    void UnregisterRequestBroker(const std::string& name) override;
    void UnregisterAllRequestBroker() override;
    RequestBrokerPtr GetRequestBroker(const std::string& name) override;
    std::vector<RequestBrokerPtr> AllRequestBroker() override;
    unsigned CreateSubscription(EventSubscriber *sub) override;
    bool SubscribeForEvent(unsigned subid, EVENT_t event) override;
    void RevokeSubscription(unsigned subid) override;
    void RevokeAllSubscriptions(EventSubscriber *sub) override;
    void DispatchEvent(const EventMessagePtr& msg) override;

  private:
    OS::Mutex m_mutex;
    OS::ThreadPool m_threadpool;
    TcpServerSocket *m_socket;

    // About subscriptions
    typedef std::map<EVENT_t, std::list<unsigned> > subscriptionsByEvent_t;
    subscriptionsByEvent_t m_subscriptionsByEvent;
    typedef std::map<unsigned, SubscriptionHandlerThread*> subscriptions_t;
    subscriptions_t m_subscriptions;

    void* process(void) override;
    void AnnounceStatus(const char *status);

    typedef std::vector<RequestBrokerPtr> RBList;
    Locked<RBList> m_RBList;
  };
}

BasicEventHandler::BasicEventHandler(const EventHandlerConf& conf)
: EventHandlerThread(conf), OS::Thread()
, m_socket(new TcpServerSocket)
, m_RBList(RBList())
{
  m_listenerAddress = EVENTHANDLER_LOOP_ADDRESS;
  m_threadpool.set_max_size(conf.threadPoolSize);
  m_threadpool.set_keep_alive(conf.threadKeepAliveMs);

  m_threadpool.start();
}

BasicEventHandler::~BasicEventHandler()
{
  BasicEventHandler::Stop();
  BasicEventHandler::UnregisterAllRequestBroker();
  m_threadpool.suspend();
  {
    OS::LockGuard lock(m_mutex);
    for (subscriptions_t::iterator it = m_subscriptions.begin(); it != m_subscriptions.end(); ++it)
      delete it->second;
    m_subscriptions.clear();
    m_subscriptionsByEvent.clear();
  }
  if (m_socket)
    delete m_socket;
  m_socket = nullptr;
}

bool BasicEventHandler::Start()
{
  if (OS::Thread::is_running())
    return true;
  return OS::Thread::start_thread();
}

void BasicEventHandler::Stop()
{
  if (OS::Thread::is_running())
  {
    DBG(DBG_DEBUG, "%s: event handler thread (%p)\n", __FUNCTION__, this);
    OS::Thread::stop_thread(true);
    DBG(DBG_DEBUG, "%s: event handler thread (%p) stopped\n", __FUNCTION__, this);
  }
}

bool BasicEventHandler::HasStarted()
{
  return OS::Thread::is_running();
}

unsigned BasicEventHandler::GetCapacity()
{
  return m_threadpool.max_size();
}

unsigned BasicEventHandler::PendingRequest()
{
  return m_threadpool.queue_size();
}

void BasicEventHandler::EnqueueRequest(EventBroker* eb)
{
  m_threadpool.enqueue(eb);
}

void BasicEventHandler::RegisterRequestBroker(RequestBrokerPtr rb)
{
  if (!rb)
    return;
  DBG(DBG_DEBUG, "%s: register (%s)\n", __FUNCTION__, rb->CommonName());
  Locked<RBList>::pointer p = m_RBList.GetExclusive();
  for (RBList::iterator it = p->begin(); it != p->end(); ++it)
    if (strcmp(rb->CommonName(), (*it)->CommonName()) == 0)
      return;
  p->push_back(rb);
}

void BasicEventHandler::UnregisterRequestBroker(const std::string &name)
{
  DBG(DBG_DEBUG, "%s: unregister (%s)\n", __FUNCTION__, name.c_str());
  Locked<RBList>::pointer p = m_RBList.GetExclusive();
  RBList keep;
  for (RBList::iterator it = p->begin(); it != p->end(); ++it)
  {
    if (name == (*it)->CommonName())
      (*it)->Abort();
    else
      keep.push_back(*it);
  }
  *p = keep;
}

void BasicEventHandler::UnregisterAllRequestBroker()
{
  Locked<RBList>::pointer p = m_RBList.GetExclusive();
  for (RBList::iterator it = p->begin(); it != p->end(); ++it)
  {
    DBG(DBG_DEBUG, "%s: unregister (%s)\n", __FUNCTION__, (*it)->CommonName());
    (*it)->Abort();
  }
  p->clear();
}

RequestBrokerPtr BasicEventHandler::GetRequestBroker(const std::string &name)
{
  Locked<RBList>::const_pointer p = m_RBList.GetShared();
  for (RBList::const_iterator it = p->cbegin(); it != p->cend(); ++it)
  {
    if (name == (*it)->CommonName())
      return *it;
  }
  return RequestBrokerPtr();
}

std::vector<RequestBrokerPtr> BasicEventHandler::AllRequestBroker()
{
  Locked<RBList>::const_pointer p = m_RBList.GetShared();
  return *p;
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
  std::list<unsigned>& sevt = m_subscriptionsByEvent[msg->event];
  std::list<unsigned>::iterator itsevt = sevt.begin();
  std::list<unsigned>::iterator itsend = sevt.end();
  while (itsevt != itsend)
  {
    subscriptions_t::const_iterator itsub = m_subscriptions.find(*itsevt);
    if (itsub != m_subscriptions.end())
      itsub->second->PostMessage(msg);
    else
      revoked.push_back(itsevt);
    ++itsevt;
  }
  std::vector<std::list<unsigned>::iterator>::const_iterator itr;
  for (itr = revoked.begin(); itr != revoked.end(); ++itr)
    m_subscriptionsByEvent[msg->event].erase(*itr);
}

void *BasicEventHandler::process()
{
  bool bound = false;

  if (m_socket->Create(SOCKET_AF_INET4))
  {
    for (int retry = 0; retry < 10; ++retry)
    {
      DBG(DBG_INFO, "%s: bind port %u\n", __FUNCTION__, m_conf.bindingPort);
      if ((bound = m_socket->Bind(m_conf.bindingPort)))
        break;
      m_conf.bindingPort += 1;
    }
  }

  if (bound)
  {
    DBG(DBG_INFO, "%s: start listening\n", __FUNCTION__);
    bound = m_socket->ListenConnection(m_conf.listenerQueueSize);
  }

  if (bound)
  {
    AnnounceStatus(EVENTHANDLER_STARTED);
    while (!OS::Thread::is_stopped())
    {
      // do not accept incoming requests when the queue is full
      if (m_threadpool.queue_size() >= m_conf.requestQueueSize)
      {
        DBG(DBG_WARN, "%s: exceeding limit on the number of pending requests (%u)\n",
            __FUNCTION__, m_conf.requestQueueSize);
        pause(EVENTHANDLER_LIMIT_HOLD_MS);
        continue;
      }

      int error = (-1);
      TcpServerSocket::AcceptStatus r = TcpServerSocket::ACCEPT_ERROR;
      {
        TcpSocket* sock = new TcpSocket();
        r = m_socket->AcceptConnection(*sock, 1000);
        if (r == TcpServerSocket::ACCEPT_SUCCESS)
        {
          DBG(DBG_DEBUG, "%s: accepting new connection\n", __FUNCTION__);
          EventBroker* eb = new EventBroker(this, sock, m_socket->GetRemoteAddrInfo());
          m_threadpool.enqueue(eb);
          continue;
        }
        error = sock->GetErrNo();
        delete sock;
      }

      if (r == TcpServerSocket::ACCEPT_FAILURE)
      {
        DBG(DBG_WARN, "%s: accept failed (%d)\n", __FUNCTION__, error);
        continue;
      }
      if (r == TcpServerSocket::ACCEPT_TIMEOUT)
      {
        continue;
      }
      DBG(DBG_ERROR, "%s: socket error (%d)\n", __FUNCTION__, m_socket->GetErrNo());
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
  msg->subject.push_back(std::to_string((uint16_t)m_conf.bindingPort));
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
  EventHandlerConf conf;
  conf.bindingPort = bindingPort;
  // Choose implementation
  m_imp = EventHandlerThreadPtr(new BasicEventHandler(conf));
}
