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

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "local_config.h"

#include <cstring>
#include <cassert>
#include <vector>

namespace NSROOT
{

class FramePacket
{
public:
  FramePacket(unsigned _id, int _size, char * _data) : id(_id), size(_size), data(_data) { }
  ~FramePacket() { if (data) delete [] data; }
  const unsigned id;
  const int size;
  const char * data;
private:
  // prevent copy
  FramePacket(const FramePacket& other);
  FramePacket& operator=(const FramePacket& other);
};

class FrameBuffer
{
public:
  FrameBuffer(int capacity);
  virtual ~FrameBuffer();

  int capacity() const;

  int bytesAvailable() const;

  void clear();

  int write(const char * data, int len);

  /**
   * Returned pointer MUST BE freed by caller
   * @see freePacket(FramePacket *)
   * @return new FramePacket or nullptr
   */
  FramePacket * read();

  void freePacket(FramePacket * p)
  {
    if (p)
      delete p;
  }

private:
  // Prevent copy
  FrameBuffer(const FrameBuffer& other);
  FrameBuffer& operator=(const FrameBuffer& other);

private:
  struct Lockable;
  mutable Lockable * m_lock;
  const int m_capacity;           /// buffer size
  volatile unsigned m_count;      /// total count of processed frame

  struct Frame
  {
    Frame() : id(0), size(0), data(nullptr), next(nullptr) { }
    ~Frame() { if (data) delete [] data; }
    unsigned id;
    int size;
    char * data;
    Frame * next;
  };

  std::vector<Frame*> m_buffer;   /// buffer of frames
  volatile Frame * m_read;        /// frame to read
  volatile Frame * m_write;       /// frame to write

  void init();
};

}

#endif /* FRAMEBUFFER_H */
