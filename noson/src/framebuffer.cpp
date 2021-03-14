/*
 *      Copyright (C) 2021 Jean-Luc Barriere
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

namespace NSROOT
{
  struct FrameBuffer::Lockable
  {
    OS::CMutex mutex;
  };
}

FramePacket::FramePacket(int _capacity)
: id(0)
, size(_capacity)
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
: m_lock(new Lockable())
, m_capacity(capacity)
, m_count(0)
, m_buffer()
, m_read(nullptr)
, m_write(nullptr)
, m_pool()
{
  assert(capacity > 0);
  m_buffer.resize(capacity);
  init();
}

FrameBuffer::~FrameBuffer()
{
  OS::CLockGuard g(m_lock->mutex);
  for (std::vector<Frame*>::iterator it = m_buffer.begin(); it != m_buffer.end(); ++it)
    delete *it;
  while (!m_pool.empty())
  {
    delete m_pool.front();
    m_pool.pop_front();
  }
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
  OS::CLockGuard g(m_lock->mutex);
  return (m_read != m_write ? m_read->packet->size : 0);
}

void FrameBuffer::clear()
{
  OS::CLockGuard g(m_lock->mutex);
  m_count = 0;
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
      OS::CLockGuard g(m_lock->mutex);
      if (m_write->packet)
        freePacket(m_write->packet);
      m_write->packet = _packet;
      m_write->packet->id = ++m_count;
      m_write = m_write->next;
    }
  }
  return len;
}

FramePacket * FrameBuffer::read()
{
  FramePacket * p = nullptr;
  {
    OS::CLockGuard g(m_lock->mutex);
    if (m_read != m_write)
    {
      p = m_read->packet;
      m_read->packet = nullptr;
      m_read = m_read->next;
    }
  }
  return p;
}

void FrameBuffer::freePacket(FramePacket * p)
{
  m_pool.push_back(p);
}

FramePacket * FrameBuffer::needPacket(int size)
{
  FramePacket * p = nullptr;
  if (!m_pool.empty())
  {
    p = m_pool.front();
    m_pool.pop_front();
    if (p->capacity >= size)
    {
      p->id = 0;
      return p;
    }
    delete p;
  }
  return new FramePacket(size);
}
