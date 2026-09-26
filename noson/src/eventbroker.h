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

#ifndef EVENTBROKER_H
#define	EVENTBROKER_H

#include "local_config.h"
#include "private/os/threads/threadpool.h"
#include "private/socket.h"
#include "eventhandler.h"

namespace NSROOT
{

  class EventBroker : public OS::Worker
  {
  public:
    EventBroker(EventHandlerThread* handler, TcpSocket* sock, const std::string& rhost);
    virtual ~EventBroker();
    virtual void process();

  private:
    EventHandlerThread* m_handler;
    std::string m_rhost;
    TcpSocket* m_sock;
    
    static TcpSocket* processEvent(
            EventHandlerThread* handler,
            TcpSocket* sock,
            const std::string& rhost);
  };
}


#endif	/* EVENTBROKER_H */

