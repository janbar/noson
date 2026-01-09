/*
 *      Copyright (C) 2018-2026 Jean-Luc Barriere
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

#include "iostream.h"

#include "private/os/threads/condition.h"
#include "private/os/threads/timeout.h"
#include "private/os/threads/mutex.h"
#include "private/debug.h"
#include "framebuffer.h"

using namespace NSROOT;

IOStream::IOStream()
: m_lock(new OS::Mutex())
, m_readyRead(new OS::Condition<bool>())
, m_open(false)
, m_output(nullptr)
{
}

IOStream::~IOStream()
{
  m_open = false;
  delete m_readyRead;
  delete m_lock;
}

int IOStream::read(char* data, int maxlen, unsigned timeout)
{
  if (!canRead())
    return -1;

  m_lock->Lock();
  if (bytesAvailable() == 0)
  {
    OS::Timeout _timeout(timeout);
    bool signaled = m_readyRead->Wait(*m_lock, _timeout);
    if (!signaled)
    {
      m_lock->Unlock();
      return 0;
    }
  }
  int r = readData(data, maxlen);
  m_lock->Unlock();
  return r;
}

int IOStream::write(const char* data, int len)
{
  if (!canWrite())
    return -1;
  return writeData(data, len);
}

void IOStream::readyRead()
{
  m_readyRead->Broadcast();
}

BufferedIOStream::BufferedIOStream()
: BufferedIOStream(2)
{
}

BufferedIOStream::BufferedIOStream(int capacity)
: IOStream()
, m_buffer(nullptr)
, m_packet(nullptr)
, m_consumed(0)
{
  m_buffer = new FrameBuffer(capacity);
}

BufferedIOStream::~BufferedIOStream()
{
  if (this->isOpen())
    this->close();
  if (m_packet)
    m_buffer->freePacket(m_packet);
  delete m_buffer;
}

bool BufferedIOStream::overflow() const
{
  return m_buffer->full();
}

int BufferedIOStream::bytesAvailable() const
{
  if (m_packet)
    return (m_packet->size - m_consumed);
  return m_buffer->bytesAvailable();
}

int BufferedIOStream::readData(char * data, int maxlen)
{
  if (m_packet == nullptr)
  {
    m_packet = m_buffer->read();
    m_consumed = 0;
  }
  if (m_packet)
  {
    int s = m_packet->size - m_consumed;
    int r = (maxlen < s ? maxlen : s);
    memcpy(data, m_packet->data + m_consumed, r);
    m_consumed += r;
    if (m_consumed >= m_packet->size)
    {
      m_buffer->freePacket(m_packet);
      m_packet = nullptr;
    }
    return r;
  }
  return 0;
}

int BufferedIOStream::writeData(const char * data, int len)
{
  // check sink: connected output, otherwise internal buffer for reading
  if (pipedTo())
    len = pipedTo()->write(data, len);
  else if ((len = m_buffer->write(data, len)) > 0)
    readyRead();
  return len;
}

void BufferedIOStream::clearBuffer()
{
  m_buffer->clear();
  if (m_packet)
    m_buffer->freePacket(m_packet);
  m_packet = nullptr;
  m_consumed = 0;
}
