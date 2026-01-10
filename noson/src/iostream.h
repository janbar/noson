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

class OutputStream
{
public:
  OutputStream() { }
  virtual ~OutputStream() { }
  virtual int write(const char * data, int len) = 0;
};

class InputStream
{
public:
  InputStream() { }
  virtual ~InputStream() { }
  virtual int read(char * data, int maxlen) = 0;
};

namespace OS
{
class Mutex;
template <typename T> class Condition;
}

class AsyncInputStream : protected InputStream
{
public:
  AsyncInputStream();
  virtual ~AsyncInputStream();
  AsyncInputStream(const AsyncInputStream& other) = delete;
  AsyncInputStream& operator=(const AsyncInputStream& other) = delete;

  int readAsync(char * data, int maxlen, unsigned timeout);

protected:
  void signalReadyRead();

    virtual int bytesAvailable() const  = 0;
    //virtual int read(char * data, int maxlen) = 0;

private:
  mutable OS::Mutex * m_lock;
  OS::Condition<bool> * m_readyRead;
};

class RingBuffer;
class RingBufferPacket;

class BufferedStream : public OutputStream, public AsyncInputStream
{
public:
  BufferedStream(int capacity);
  virtual ~BufferedStream() override;

  int bytesAvailable() const override;
  bool overflow() const;

  int write(const char * data, int len) override;
  //int readAsync(char * data, int maxlen, unsigned timeout);

protected:
  int read(char * data, int maxlen) override;
  void clearBuffer();

private:
  RingBuffer * m_buffer;
  RingBufferPacket * m_packet;
  int m_consumed;
};

}

#endif /* IOSTREAM_H */

