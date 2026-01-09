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

#ifndef AUDIOSOURCE_H
#define AUDIOSOURCE_H

#include "local_config.h"
#include "iostream.h"
#include "audioformat.h"

#include <string>

namespace NSROOT
{

class FrameBuffer;
class FramePacket;

class AudioSource : public BufferedIOStream
{
public:
  AudioSource();
  AudioSource(int capacity);
  virtual ~AudioSource() override;

  bool canRead() const override { return true; }
  bool canWrite() const override { return false; }

  virtual bool open() override;

  /**
   * Return the name of the source
   * @return the name
   */
  virtual std::string getName() const = 0;

  /**
   * Return the description of the source
   * @return the description
   */
  virtual std::string getDescription() const = 0;

  /**
   * Return the audio format that the source provides
   * @return the audio format
   */
  virtual AudioFormat getFormat() const = 0;

  void mute(bool enabled);
  bool muted() const { return m_mute; }

protected:
  volatile bool m_mute;
};

}

#endif /* AUDIOSOURCE_H */

