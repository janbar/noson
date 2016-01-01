/*
 *      Copyright (C) 2014-2015 Jean-Luc Barriere
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

#include "subscription.h"
#include "private/cppdef.h"
#include "private/os/threads/thread.h"
#include "private/os/threads/event.h"
#include "private/wsresponse.h"
#include "private/uriparser.h"
#include "private/socket.h"
#include "private/builtin.h"
#include "sonossystem.h" // for definitions

#define TIMEOUT_RETRY  1 // seconds

using namespace NSROOT;

Subscription::SubscriptionThread::SubscriptionThread()
{
}

Subscription::SubscriptionThread::~SubscriptionThread()
{
}

namespace NSROOT
{

  class SubscriptionThreadImpl : public Subscription::SubscriptionThread, private OS::CThread
  {
  public:
    SubscriptionThreadImpl(const std::string& host, unsigned port, const std::string& url, unsigned bindingPort, unsigned timeout)
    : CThread()
    , m_host(host)
    , m_port(port)
    , m_url(url)
    , m_bindingPort(bindingPort)
    , m_timeout(timeout)
    , m_configured(false)
    {
      Configure();
    }

    virtual ~SubscriptionThreadImpl()
    {
      Stop();
    }

    virtual bool IsValid()
    {
      return m_configured;
    }

    virtual bool Start()
    {
      return OS::CThread::StartThread();
    }

    virtual void Stop()
    {
      OS::CThread::StopThread(false);
      m_event.Signal();
      OS::CThread::StopThread(true);
    }

    virtual bool IsRunning() { return OS::CThread::IsRunning(); }

    virtual void AskRenewal()
    {
      if (IsRunning())
      {
        UnSubscribeForEvent();
        m_event.Signal();
      }
    }

  private:
    std::string m_host;
    unsigned m_port;
    std::string m_url;
    unsigned m_bindingPort;
    unsigned m_timeout;
    bool m_configured;
    std::string m_myIP;
    OS::CEvent m_event;

    virtual void* Process();
    void Configure();
    bool SubscribeForEvent(bool renew = false);
    bool UnSubscribeForEvent();
  };
}

void* SubscriptionThreadImpl::Process()
{
  bool success = false;
  while (!IsStopped())
  {
    if (m_configured && (success = SubscribeForEvent(success)))
      m_event.Wait(m_timeout * 900);
    else
    {
      // reconfigure and wait before retry
      Configure();
      m_event.Wait(TIMEOUT_RETRY * 1000);
    }
  }
  if (success)
    UnSubscribeForEvent();
  return NULL;
}

void SubscriptionThreadImpl::Configure()
{
  TcpSocket sock;
  sock.Connect(m_host.c_str(), m_port, 0);
  m_myIP = sock.GetLocalIP(),
  sock.Disconnect();
  if (!m_myIP.empty())
    m_configured = true;
  else
    m_configured = false;
}

bool SubscriptionThreadImpl::SubscribeForEvent(bool renew)
{
  WSRequest request(m_host, m_port);
  request.RequestService(m_url, HRM_SUBSCRIBE);
  if (!m_SID.empty() && renew)
    request.SetHeader("SID", m_SID);
  else
  {
    char buf[11];
    memset(buf, 0, sizeof(buf));
    uint16_to_string(m_bindingPort, buf);
    request.SetHeader("Callback", "<http://" + m_myIP + ":" + buf + ">");
    request.SetHeader("NT", "upnp:event");
  }
  char buf[18];
  memset(buf, 0, sizeof(buf));
  strncpy(buf, "Second-0", 8);
  uint32_t secs = (uint32_t)m_timeout;
  uint32_to_string(secs, buf + 7);
  request.SetHeader("TIMEOUT", buf);
  WSResponse response(request);
  if (response.IsSuccessful() && response.GetHeaderValue("SID", m_SID))
    return true;
  return false;
}

bool SubscriptionThreadImpl::UnSubscribeForEvent()
{
  if (!m_SID.empty())
  {
    WSRequest request(m_host, m_port);
    request.RequestService(m_url, HRM_UNSUBSCRIBE);
    request.SetHeader("SID", m_SID);
    WSResponse response(request);
    if (!response.IsSuccessful())
      return false;
    m_SID.clear();
  }
  return true;
}

Subscription::Subscription()
: m_imp()
{
}

Subscription::Subscription(const std::string& host, unsigned port, const std::string& url, unsigned bindingPort, unsigned timeout)
: m_imp()
{
  m_imp = SubscriptionThreadPtr(new SubscriptionThreadImpl(host, port, url, bindingPort, timeout));
}

Subscription::~Subscription()
{
}

bool Subscription::IsValid()
{
  return m_imp ? m_imp->IsValid() : false;
}

bool Subscription::Start()
{
  if (m_imp)
    return m_imp->Start();
  return false;
}

void Subscription::AskRenewal()
{
  if (m_imp)
    m_imp->AskRenewal();
}
