/*
 *      Copyright (C) 2018-2019 Jean-Luc Barriere
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

#include "audiosource.h"
#include "framebuffer.h"

using namespace NSROOT;

#define FRAME_BUFFER_SIZE 256

AudioSource::AudioSource()
{
  m_buffer = new FrameBuffer(FRAME_BUFFER_SIZE);
}

AudioSource::AudioSource(int buffered)
{
  m_buffer = new FrameBuffer(buffered);
}

AudioSource::~AudioSource()
{
  if (m_packet)
    m_buffer->freePacket(m_packet);
  delete m_buffer;
}

bool AudioSource::startRecording()
{
  if (m_packet)
    m_buffer->freePacket(m_packet);
  m_packet = nullptr;
  m_buffer->clear();
  m_record = true;
  return true;
}

void AudioSource::stopRecording()
{
  m_record = false;
}

bool AudioSource::overflow() const
{
  return m_buffer->full();
}

int AudioSource::bytesAvailable() const
{
  if (m_packet)
    return (m_packet->size - m_consumed);
  return m_buffer->bytesAvailable();
}

void AudioSource::mute(bool enabled)
{
  m_mute = enabled;
}

int AudioSource::readData(char * data, int maxlen)
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

int AudioSource::writeData(const char * data, int len)
{
  if (m_record)
  {
    // check sink: connected output, otherwise internal buffer for reading
    IODevice* output = connectedOutput();
    if (output)
      len = output->write(data, len);
    else if ((len = m_buffer->write(data, len)) > 0)
      readyRead();
  }
  return len;
}
