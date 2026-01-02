#include <iostream>

#include "include/testmain.h"
#include "locked.h"

#include <private/os/threads/threadpool.h>
#include <noson/intrinsic.h>
#include <noson/locked.h>

SONOS::IntrinsicCounter* g_counter = 0;

class WorkerInc : public SONOS::OS::Worker
{
  virtual void Process()
  {
    for (int i = 0; i < 5000100; i++)
    {
      g_counter->Increment();
    }
  }
};

class WorkerDec : public SONOS::OS::Worker
{
  virtual void Process()
  {
    for (int i = 0; i < 5000000; i++)
    {
      g_counter->Decrement();
    }
  }
};

TEST_CASE("Stress atomic counter")
{
  int val = 0;
  g_counter = new SONOS::IntrinsicCounter(val);
  SONOS::OS::ThreadPool pool(4);
  pool.Suspend();
  pool.Enqueue(new WorkerInc());
  pool.Enqueue(new WorkerDec());
  pool.SetKeepAlive(100);
  pool.Resume();
  unsigned ps;
  while ((ps = pool.Size()) > 0)
    usleep(100000);
  REQUIRE(g_counter->GetValue() == (val+100));
  delete g_counter;
}

SONOS::LockedNumber<int>* g_locked;

class WorkerLockInc : public SONOS::OS::Worker
{
  virtual void Process()
  {
    for (int i = 0; i < 500100; i++)
    {
      g_locked->Add(1);
    }
  }
};

class WorkerLockDec : public SONOS::OS::Worker
{
  virtual void Process()
  {
    for (int i = 0; i < 500000; i++)
    {
      g_locked->Sub(1);
    }
  }
};

TEST_CASE("Stress locked number")
{
  int val = 0;
  g_locked = new SONOS::LockedNumber<int>(val);
  SONOS::OS::ThreadPool pool(4);
  pool.Suspend();
  pool.Enqueue(new WorkerLockInc());
  pool.Enqueue(new WorkerLockDec());
  pool.SetKeepAlive(100);
  pool.Resume();
  unsigned ps;
  while ((ps = pool.Size()) > 0)
    usleep(100000);
  REQUIRE(g_locked->Load() == (val+100));
  delete g_locked;
}
