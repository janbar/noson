/*
 *      Copyright (C) 2022 Jean-Luc Barriere
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "ringbuffer.h"

#include "os/threads/mutex.h"
#include "debug.h"

using namespace NSROOT;

RingBufferPacket::RingBufferPacket(int _capacity)
: id(0)
, size(0)
, data(new char [_capacity])
, capacity(_capacity)
{
}

RingBufferPacket::~RingBufferPacket()
{
  if (data)
    delete [] data;
}

RingBuffer::RingBuffer(int capacity)
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
  assert(capacity > 0);
  m_buffer.resize(capacity);
  init();
}

RingBuffer::~RingBuffer()
{
  m_ringlock->Lock();
  for (std::vector<Chunk*>::iterator it = m_buffer.begin(); it != m_buffer.end(); ++it)
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

void RingBuffer::init()
{
  Chunk * previous = nullptr;
  for (std::vector<Chunk*>::iterator it = m_buffer.begin(); it != m_buffer.end(); ++it)
  {
    *it = new Chunk();
    if (previous)
      previous->next = *it;
    previous = *it;
  }
  if (m_buffer.begin() != m_buffer.end())
    previous->next = *(m_buffer.begin());
  m_write = *(m_buffer.begin());
  m_read = m_write;
}

int RingBuffer::capacity() const
{
  return m_capacity;
}

int RingBuffer::bytesAvailable() const
{
  OS::LockGuard g(*m_ringlock);
  return (m_unread ? m_read->packet->size : 0);
}

unsigned RingBuffer::bytesUnread() const
{
  OS::LockGuard g(*m_ringlock);
  return m_unread;
}

bool RingBuffer::full() const
{
  OS::LockGuard g(*m_ringlock);
  return (m_unread && m_read == m_write);
}

void RingBuffer::clear()
{
  OS::LockGuard g(*m_ringlock);
  // reset of unread implies the reset of packet size
  // so clean all chunks in the buffer
  for (std::vector<Chunk*>::iterator it = m_buffer.begin(); it != m_buffer.end(); ++it)
  {
    if ((*it)->packet)
      freePacket((*it)->packet);
    (*it)->packet = nullptr;
  }
  m_count = m_unread = 0;
  m_read = m_write;
}

int RingBuffer::write(const char * data, int len)
{
  if (len > 0)
  {
    RingBufferPacket * _packet = needPacket(len);
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

RingBufferPacket* RingBuffer::newPacket(int len)
{
  RingBufferPacket * _packet = needPacket(len);
  _packet->size = 0;
  return _packet;
}

void RingBuffer::writePacket(RingBufferPacket* packet)
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

RingBufferPacket * RingBuffer::read()
{
  RingBufferPacket * p = nullptr;
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

void RingBuffer::freePacket(RingBufferPacket * p)
{
  m_poollock->Lock();
  m_pool.push_back(p);
  m_poollock->Unlock();
}

RingBufferPacket * RingBuffer::needPacket(int size)
{
  RingBufferPacket * p = nullptr;
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
  p = new RingBufferPacket(size);
  //DBG(DBG_DEBUG, "%s: allocated packet to buffer (%d)\n", __FUNCTION__, p->capacity);
  return p;
}
