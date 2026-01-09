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

#ifndef IOSTREAM_H
#define IOSTREAM_H

#include "local_config.h"

namespace NSROOT
{

namespace OS
{
class Mutex;
template <typename T> class Condition;
}

class IOStream
{
public:
  IOStream();
  virtual ~IOStream();
  IOStream(const IOStream& other) = delete;
  IOStream& operator=(const IOStream& other) = delete;

  virtual bool canRead() const = 0;
  virtual bool canWrite() const = 0;
  virtual int bytesAvailable() const  = 0;
  
  virtual bool open() { return (m_open = true); }
  virtual void close() { m_open = false; }
  virtual bool isOpen() { return m_open; }
  
  int read(char * data, int maxlen, unsigned timeout);
  int write(const char * data, int len);

  void pipeTo(IOStream * s) { m_output = s; }
  IOStream* pipedTo() const { return m_output; }

protected:
  void readyRead();
  virtual int readData(char * data, int maxlen) = 0;
  virtual int writeData(const char * data, int len) = 0;

private:
  mutable OS::Mutex * m_lock;
  OS::Condition<bool> * m_readyRead;
  bool m_open;
  IOStream * m_output;
};

class FrameBuffer;
class FramePacket;

class BufferedIOStream : public IOStream
{
public:
  BufferedIOStream();
  BufferedIOStream(int capacity);
  virtual ~BufferedIOStream() override;

  int bytesAvailable() const override;
  bool overflow() const;

protected:
  int readData(char * data, int maxlen) override;
  int writeData(const char * data, int len) override;
  void clearBuffer();

private:
  FrameBuffer * m_buffer;
  FramePacket * m_packet;
  int m_consumed;
};

}

#endif /* IOSTREAM_H */

