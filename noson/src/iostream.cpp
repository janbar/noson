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
#include "private/ringbuffer.h"
#include "private/debug.h"

using namespace NSROOT;

AsyncInputStream::AsyncInputStream()
: m_lock(new OS::Mutex())
, m_readyRead(new OS::Condition<bool>())
{
}

AsyncInputStream::~AsyncInputStream()
{
  delete m_readyRead;
  delete m_lock;
}

int AsyncInputStream::ReadAsync(char* data, int maxlen, unsigned timeout)
{
  m_lock->Lock();
  if (BytesAvailable() == 0)
  {
    OS::Timeout _timeout(timeout);
    bool signaled = m_readyRead->Wait(*m_lock, _timeout);
    if (!signaled)
    {
      m_lock->Unlock();
      return 0;
    }
  }
  int r = Read(data, maxlen);
  m_lock->Unlock();
  return r;
}

void AsyncInputStream::SignalReadyRead()
{
  m_readyRead->Broadcast();
}

BufferedStream::BufferedStream(int capacity)
: OutputStream(), AsyncInputStream()
, m_buffer(nullptr)
, m_packet(nullptr)
, m_consumed(0)
{
  m_buffer = new RingBuffer(capacity);
}

BufferedStream::~BufferedStream()
{
  if (m_packet)
    m_buffer->freePacket(m_packet);
  delete m_buffer;
}

bool BufferedStream::Overflow() const
{
  return m_buffer->full();
}

int BufferedStream::BytesAvailable() const
{
  if (m_packet)
    return (m_packet->size - m_consumed);
  return m_buffer->bytesAvailable();
}

int BufferedStream::Read(char * data, int maxlen)
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

int BufferedStream::Write(const char * data, int len)
{
  if ((len = m_buffer->write(data, len)) > 0)
    SignalReadyRead();
  return len;
}

void BufferedStream::ClearBuffer()
{
  m_buffer->clear();
  if (m_packet)
    m_buffer->freePacket(m_packet);
  m_packet = nullptr;
}
