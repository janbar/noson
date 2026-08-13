#if (defined(_WIN32) || defined(_WIN64))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__
#include <WinSock2.h>
#include <Windows.h>
#include <time.h>
//#define usleep(t) Sleep((DWORD)(t)/1000)
//#define sleep(t)  Sleep((DWORD)(t)*1000)
#else
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#endif

#include "private/wsresponse.h"
#include "private/debug.h"
#include "private/securesocket.h"

#include <string.h>
#include <cstdio>

#ifndef __WINDOWS__
static void signalHandler(int signal, siginfo_t * info, void * data)
{
  SONOS::DBG(DBG_DEBUG, "signal %d catched\n", signal);
  (void)info;
  (void)data;
}

static bool catchSignal(int signal)
{
  struct sigaction act;
  memset(&act, '\0', sizeof(struct sigaction));
  act.sa_sigaction = &signalHandler;
  act.sa_flags |= SA_SIGINFO;
  sigemptyset(&act.sa_mask);
  return (sigaction(signal, &act, 0) == 0);
}
#endif

int main(int argc, char** argv) {

  int ret = 0;
#ifdef __WINDOWS__
  //Initialize Winsock
  WSADATA wsaData;
  if ((ret = WSAStartup(MAKEWORD(2, 2), &wsaData)))
    return ret;
#else
  catchSignal(SIGPIPE);
#endif /* __WINDOWS__ */

  const char* dest_host = "www.google.fr";
  int dest_port = 443;
  if (argc > 1)
    dest_host = argv[1];
  if (argc > 2)
    dest_port = atoi(argv[2]);

  SONOS::DBGLevel(4);

  SONOS::WSRequest req(dest_host, dest_port, true);
  req.RequestAcceptEncoding(true);
  req.RequestService("/", WS_METHOD_Get);
  SONOS::WSResponse resp(req);
  if (resp.IsSuccessful())
  {
    int l = 0;
    char buf[500];
    while ((l = resp.ReadContent(buf, 500)) > 0) {
      fwrite(buf, l, 1, stdout);
    }
  }

  SONOS::SSLSessionFactory::Destroy();

  //out:
#ifdef __WINDOWS__
  WSACleanup();
#endif /* __WINDOWS__ */
  return ret;
}
