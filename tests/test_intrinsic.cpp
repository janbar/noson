
#include <private/os/threads/threadpool.h>
#include <private/os/threads/atomic.h>
#include <private/os/threads/latch.h>
#include <noson/sharedptr.h>
#include <noson/locked.h>

#include <iostream>

#include <test.h>

SONOS::OS::Atomic* g_counter = nullptr;

class WorkerInc : public SONOS::OS::Worker
{
  virtual void process()
  {
    for (int i = 0; i < 5000100; i++)
    {
      g_counter->increment();
    }
  }
};

class WorkerDec : public SONOS::OS::Worker
{
  virtual void process()
  {
    for (int i = 0; i < 5000000; i++)
    {
      g_counter->decrement();
    }
  }
};

TEST_CASE("Stress atomic counter")
{
  int val = 0;
  g_counter = new SONOS::OS::Atomic(val);
  SONOS::OS::ThreadPool pool(4);
  pool.suspend();
  pool.enqueue(new WorkerInc());
  pool.enqueue(new WorkerDec());
  pool.set_keep_alive(100);
  pool.resume();
  unsigned ps;
  while ((ps = pool.size()) > 0)
    usleep(100000);
  REQUIRE(g_counter->load() == (val+100));
  delete g_counter;
}

SONOS::Locked<int>* g_locked;

class WorkerLockInc : public SONOS::OS::Worker
{
  virtual void process()
  {
    for (int i = 0; i < 500100; i++)
    {
      SONOS::Locked<int>::pointer p = g_locked->Get();
      *p += 1;
    }
  }
};

class WorkerLockDec : public SONOS::OS::Worker
{
  virtual void process()
  {
    for (int i = 0; i < 500000; i++)
    {
      SONOS::Locked<int>::pointer p = g_locked->Get();
      *p -= 1;
    }
  }
};

TEST_CASE("Stress locked object")
{
  int val = 0;
  g_locked = new SONOS::Locked<int>(val);
  SONOS::OS::ThreadPool pool(4);
  pool.suspend();
  pool.enqueue(new WorkerLockInc());
  pool.enqueue(new WorkerLockDec());
  pool.set_keep_alive(100);
  pool.resume();
  unsigned ps;
  while ((ps = pool.size()) > 0)
    usleep(100000);
  REQUIRE(g_locked->Load() == (val+100));
  delete g_locked;
}

SONOS::OS::Latch g_latch(false);
SONOS::shared_ptr<size_t> g_pointer;

class WorkerPtrClear : public SONOS::OS::Worker
{
  virtual void process()
  {
    for (int i = 0; i < 10000; i++)
    {
      g_latch.lock();
      g_pointer.reset(new size_t(i));
      g_latch.unlock();
      usleep(1);
    }
  }
};

class WorkerPtrCopy : public SONOS::OS::Worker
{
  virtual void process()
  {
    for (int i = 0; i < 10000; i++)
    {
      g_latch.lock_shared();
      SONOS::shared_ptr<size_t> ptr(g_pointer);
      g_latch.unlock_shared();
      if (ptr)
        g_counter->increment();
      usleep(1);
    }
  }
};

TEST_CASE("Stress shared pointer")
{
  int target = 30000;
  g_counter = new SONOS::OS::Atomic(0);
  g_pointer.reset(new size_t(0));
  SONOS::OS::ThreadPool pool(4);
  pool.suspend();
  pool.enqueue(new WorkerPtrCopy());
  pool.enqueue(new WorkerPtrClear());
  pool.enqueue(new WorkerPtrCopy());
  pool.enqueue(new WorkerPtrCopy());
  pool.set_keep_alive(100);
  pool.resume();
  unsigned ps;
  while ((ps = pool.size()) > 0)
  {
    std::cout << "... Running: " << ps
              << " , " << ((100*g_counter->load())/target) << "%"
              << std::endl;
    usleep(250000);
  }
  std::cout << "Count   = " << g_counter->load() << std::endl;
  std::cout << "Payload = " << *g_pointer << std::endl;
  REQUIRE(g_counter->load() == target);
  REQUIRE((*g_pointer) == 9999);
  delete g_counter;
}
