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

#include "eventbroker.h"
#include "private/wsrequestreply.h"
#include "private/wsstatic.h"
#include "private/debug.h"

using namespace NSROOT;

#define CONNECTION_TIMEOUT  5 // default timeout in seconds

EventBroker::EventBroker(EventHandlerThread* handler, TcpSocket* sock, const std::string& rhost)
: m_handler(handler)
, m_rhost(rhost)
, m_sock(sock)
{
}

EventBroker::~EventBroker()
{
  if (m_sock)
    delete m_sock;
}

void EventBroker::process()
{
  m_sock = processEvent(m_handler, m_sock, m_rhost);
}

TcpSocket* EventBroker::processEvent(
        EventHandlerThread* handler,
        TcpSocket* sock,
        const std::string& rhost)
{
  if (!handler || !sock || !sock->IsValid())
    return sock;

  WSRequestBroker rb(sock, false, handler->GetClientTimeout());

  if (!rb.IsParsed())
  {
    if (rb.GetRequestMethod() != WS_METHOD_UNKNOWN)
    {
      WSRequestReply rr(rb);
      rr.CloseReply(WS_STATUS_400_Bad_Request);
    }
    sock->Disconnect();
    return sock;
  }

  // override keep-alive flag with the handler configuration
  if (!handler->GetClientKeepAlive())
    rb.SetKeepAlive(false);

  RequestBroker::handle handle { handler, &rb };
  std::vector<RequestBrokerPtr> vect = handler->AllRequestBroker();
  for (std::vector<RequestBrokerPtr>::iterator itrb = vect.begin(); itrb != vect.end(); ++itrb)
  {
    // loop until the request is processed
    if ((*itrb)->HandleRequest(&handle))
    {
      // handling persistant connection (keep-alive)
      if (rb.IsKeepAlive())
      {
        handler->EnqueueRequest(new EventBroker(handler, sock, rhost));
        // the socket ownership has been transferred
        return nullptr;
      }

      sock->Disconnect();
      return sock;
    }
  }

  // processing method HEAD, otherwise it is a bad request
  if (rb.GetRequestMethod() == WS_METHOD_Head && rb.GetRequestPath().compare("/") == 0)
  {
    WSRequestReply rr(rb);
    rr.CloseReply(WS_STATUS_200_OK);
  }
  else
  {
    WSRequestReply::ReturnStatus(rb, WS_STATUS_400_Bad_Request);
  }
  sock->Disconnect();
  return sock;
}
