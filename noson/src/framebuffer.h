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
#include <list>

namespace NSROOT
{

class FramePacket
{
public:
  FramePacket(int _capacity);
  ~FramePacket();
  unsigned id;
  int size;
  char * const data;
  const int capacity;
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

  unsigned bytesUnread() const;

  bool full() const;

  void clear();

  int write(const char * data, int len);

  FramePacket * newPacket(int len);

  void writePacket(FramePacket * packet);

  /**
   * Returned pointer MUST BE freed by caller
   * @see freePacket(FramePacket *)
   * @return new FramePacket or nullptr
   */
  FramePacket * read();

  void freePacket(FramePacket * p);

private:
  // Prevent copy
  FrameBuffer(const FrameBuffer& other);
  FrameBuffer& operator=(const FrameBuffer& other);

private:
  struct Lockable;
  mutable Lockable * m_ringlock;
  mutable Lockable * m_poollock;
  const int m_capacity;           /// buffer size
  unsigned m_count;               /// total count of processed frame
  unsigned m_unread;              /// total size of unread data in the buffer

  struct Frame
  {
    Frame() : packet(nullptr), next(nullptr) { }
    ~Frame() { if (packet) delete packet; }
    FramePacket * packet;
    Frame * next;
  };

  std::vector<Frame*> m_buffer;   /// buffer of frames
  Frame * m_read;                 /// frame to read
  Frame * m_write;                /// frame to write

  void init();

  std::list<FramePacket*> m_pool;
  FramePacket * needPacket(int size);
};

}

#endif /* FRAMEBUFFER_H */
