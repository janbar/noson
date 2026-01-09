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

#include "audiosource.h"

using namespace NSROOT;

#define FRAME_BUFFER_SIZE 256

AudioSource::AudioSource()
: AudioSource(FRAME_BUFFER_SIZE)
{
}

AudioSource::AudioSource(int capacity)
: BufferedIOStream(capacity)
, m_mute(false)
{
}

AudioSource::~AudioSource()
{
  if (BufferedIOStream::isOpen())
    close();
}

bool AudioSource::open()
{
  if (BufferedIOStream::isOpen())
    return true;
  BufferedIOStream::clearBuffer();
  return BufferedIOStream::open();
}

void AudioSource::mute(bool enabled)
{
  m_mute = enabled;
}
