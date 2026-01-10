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

#ifndef AUDIOENCODER_H
#define AUDIOENCODER_H

#include "local_config.h"
#include "iostream.h"
#include "audioformat.h"

namespace NSROOT
{

class AudioEncoder : public OutputStream
{
public:
  AudioEncoder() { }
  virtual ~AudioEncoder() { }
  virtual std::string mediaType() const = 0;
  virtual bool open(const AudioFormat& format, OutputStream * out) = 0;
  virtual void close() = 0;
};

}

#endif /* AUDIOENCODER_H */

