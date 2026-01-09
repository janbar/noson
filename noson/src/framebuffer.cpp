/*
 *      Copyright (C) 2021-2026 Jean-Luc Barriere
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "framebuffer.h"

#include "private/os/threads/mutex.h"
#include "private/debug.h"

using namespace NSROOT;

FramePacket::FramePacket(int _capacity)
: id(0)
, size(0)
, data(new char [_capacity])
, capacity(_capacity)
{
}

FramePacket::~FramePacket()
{
  if (data)
    delete [] data;
}

FrameBuffer::FrameBuffer(int capacity)
: m_ringlock(new OS::Mutex())
, m_poollock(new OS::Mutex())
, m_capacity(capacity)
, m_count(0)
, m_unread(0)
, m_buffer()
, m_read(nullptr)
, m_write(nullptr)
, m_pool()
{
  // size at least 1 packet
  m_buffer.resize(capacity > 0 ? capacity : 1);
  init();
}

FrameBuffer::~FrameBuffer()
{
  m_ringlock->Lock();
  for (std::vector<Frame*>::iterator it = m_buffer.begin(); it != m_buffer.end(); ++it)
    delete *it;
  m_ringlock->Unlock();
  m_poollock->Lock();
  while (!m_pool.empty())
  {
    delete m_pool.front();
    m_pool.pop_front();
  }
  m_poollock->Unlock();
  delete m_poollock;
  delete m_ringlock;
}

void FrameBuffer::init()
{
  Frame * previous = nullptr;
  for (std::vector<Frame*>::iterator it = m_buffer.begin(); it != m_buffer.end(); ++it)
  {
    *it = new Frame();
    if (previous)
      previous->next = *it;
    previous = *it;
  }
  if (m_buffer.begin() != m_buffer.end())
    previous->next = *(m_buffer.begin());
  m_write = *(m_buffer.begin());
  m_read = m_write;
}

int FrameBuffer::capacity() const
{
  return m_capacity;
}

int FrameBuffer::bytesAvailable() const
{
  OS::LockGuard g(*m_ringlock);
  return (m_unread ? m_read->packet->size : 0);
}

unsigned FrameBuffer::bytesUnread() const
{
  OS::LockGuard g(*m_ringlock);
  return m_unread;
}

bool FrameBuffer::full() const
{
  OS::LockGuard g(*m_ringlock);
  return (m_unread && m_read == m_write);
}

void FrameBuffer::clear()
{
  OS::LockGuard g(*m_ringlock);
  // reset of unread implies the reset of packet size
  // so clean all frames in the buffer
  for (std::vector<Frame*>::iterator it = m_buffer.begin(); it != m_buffer.end(); ++it)
  {
    if ((*it)->packet)
      freePacket((*it)->packet);
    (*it)->packet = nullptr;
  }
  m_count = m_unread = 0;
  m_read = m_write;
}

int FrameBuffer::write(const char * data, int len)
{
  if (len > 0)
  {
    FramePacket * _packet = needPacket(len);
    _packet->size = len;
    memcpy(_packet->data, data, len);
    {
      OS::LockGuard g(*m_ringlock);
      if (m_write->packet)
      {
        // overwriting a packet implies to update unread because the data will be destroyed,
        // and no longer available for reading.
        m_unread -= m_write->packet->size;
        freePacket(m_write->packet);
      }
      m_write->packet = _packet;
      m_write->packet->id = ++m_count;
      m_write = m_write->next;
      m_unread += _packet->size;
    }
  }
  return len;
}

FramePacket* FrameBuffer::newPacket(int len)
{
  FramePacket * _packet = needPacket(len);
  _packet->size = 0;
  return _packet;
}

void FrameBuffer::writePacket(FramePacket* packet)
{
  if (packet)
  {
    OS::LockGuard g(*m_ringlock);
    if (m_write->packet)
    {
      // overwriting a packet implies to update unread because the data will be destroyed,
      // and no longer available for reading.
      m_unread -= m_write->packet->size;
      freePacket(m_write->packet);
    }
    m_write->packet = packet;
    m_write->packet->id = ++m_count;
    m_write = m_write->next;
    m_unread += packet->size;
  }
}

FramePacket * FrameBuffer::read()
{
  FramePacket * p = nullptr;
  {
    OS::LockGuard g(*m_ringlock);
    if (m_unread)
    {
      p = m_read->packet;
      m_read->packet = nullptr;
      m_read = m_read->next;
      m_unread -= p->size;
    }
  }
  return p;
}

void FrameBuffer::freePacket(FramePacket * p)
{
  m_poollock->Lock();
  m_pool.push_back(p);
  m_poollock->Unlock();
}

FramePacket * FrameBuffer::needPacket(int size)
{
  FramePacket * p = nullptr;
  m_poollock->Lock();
  if (!m_pool.empty())
  {
    p = m_pool.front();
    m_pool.pop_front();
    m_poollock->Unlock();
    if (p->capacity >= size)
    {
      p->id = 0;
      return p;
    }
    //DBG(DBG_DEBUG, "%s: freed packet from buffer (%d)\n", __FUNCTION__, p->capacity);
    delete p;
  }
  else
  {
    m_poollock->Unlock();
  }
  p = new FramePacket(size);
  //DBG(DBG_DEBUG, "%s: allocated packet to buffer (%d)\n", __FUNCTION__, p->capacity);
  return p;
}
