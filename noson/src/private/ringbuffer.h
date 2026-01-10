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

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "local_config.h"

#include <cstring>
#include <cassert>
#include <vector>
#include <list>

namespace NSROOT
{

namespace OS
{
class Mutex;
}

class RingBufferPacket
{
public:
  RingBufferPacket(int _capacity);
  ~RingBufferPacket();
  unsigned id;
  int size;
  char * const data;
  const int capacity;
private:
  // prevent copy
  RingBufferPacket(const RingBufferPacket& other);
  RingBufferPacket& operator=(const RingBufferPacket& other);
};

class RingBuffer
{
public:
  RingBuffer(int capacity);
  virtual ~RingBuffer();

  /**
   * Buffer capacity is defined in CTOR and it cannot be changed.
   * It is the number of chunk chained in the ring of the buffer.
   * Each chunk can hold different size of payload.
   * @return the number of chunk in the ring buffer
   */
  int capacity() const;

  /**
   * It returns the size in bytes of the next chunk to read.
   * @return the number of bytes
   */
  int bytesAvailable() const;

  /**
   * It returns the total size in bytes of all chunks remaining to be read.
   * @return the number of bytes
   */
  unsigned bytesUnread() const;

  /**
   * When the buffer is full, writing new data will overwrite the next
   * available chunk for reading.
   * @return true if the buffer is full
   */
  bool full() const;

  /**
   * Clear all unread chunks.
   */
  void clear();

  /**
   * Write a chunk in the buffer.
   * @param data pointer to data
   * @param len length of data
   * @return the number of bytes written
   */
  int write(const char * data, int len);

  /**
   * It returns a pointer to payload to be written. Its capacity is at least
   * equal to requested size (len). After copying data and assigning the size,
   * the payload can be written to the buffer. An unused payload MUST BE freed
   * by caller using freePacket(RingBufferPacket*).
   * @param len requested size in bytes
   * @return payload
   * @see writePacket(RingBufferPacket*)
   */
  RingBufferPacket * newPacket(int len);

  /**
   * Write a chunk in the buffer.
   * @param packet payload
   * @see newPacket(int)
   */
  void writePacket(RingBufferPacket * packet);

  /**
   * Read the next chunk available. Returned pointer MUST BE freed by caller.
   * @see freePacket(RingBufferPacket*)
   * @return new packet or nullptr
   */
  RingBufferPacket * read();

  /**
   * Recycle the payload for reuse.
   * @param p payload
   */
  void freePacket(RingBufferPacket * p);

private:
  // Prevent copy
  RingBuffer(const RingBuffer& other);
  RingBuffer& operator=(const RingBuffer& other);

private:
  mutable OS::Mutex * m_ringlock;
  mutable OS::Mutex * m_poollock;
  const int m_capacity;           /// buffer size
  unsigned m_count;               /// total count of processed chunk
  unsigned m_unread;              /// total size of unread data in the buffer

  struct Chunk
  {
    Chunk() : packet(nullptr), next(nullptr) { }
    ~Chunk() { if (packet) delete packet; }
    RingBufferPacket * packet;
    Chunk * next;
  };

  std::vector<Chunk*> m_buffer;   /// buffer of chunk
  Chunk * m_read;                 /// chunk to read
  Chunk * m_write;                /// chunk to write

  void init();

  std::list<RingBufferPacket*> m_pool;
  RingBufferPacket * needPacket(int size);
};

}

#endif /* RINGBUFFER_H */

