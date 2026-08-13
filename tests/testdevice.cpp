#if (defined(_WIN32) || defined(_WIN64))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__
#include <WinSock2.h>
#include <Windows.h>
#include <time.h>
#define STDOUT stdout
#define STDERR stderr
#else
#include <unistd.h>
#include <sys/time.h>
#define STDOUT stdout
#define STDERR stderr
#endif

#include <noson/sonossystem.h>
#include <noson/contentdirectory.h>
#include <noson/avtransport.h>
#include <noson/musicservices.h>
#include <noson/smapi.h>
#include <noson/didlparser.h>

#include <cstdio>
#include <string>
#include <cstdlib>

#define PRINT(a) fputs(a, STDOUT)
#define PRINTF(a, ...) fprintf(STDOUT, a, __VA_ARGS__)
#define PERROR(a) fputs(a, STDERR)
#define PERRORF(a, ...) fprintf(STDERR, a, __VA_ARGS__)

void handleEventCB(void* handle)
{
  PERROR("#########################\n");
  PERROR("### Handling event CB ###\n");
  PERROR("#########################\n");
}

void usage(const char* cmd)
{
  PERRORF(
        "Usage: %s [options]\n"
        "  --zone <zone name>         Connect to zone\n"
        "  --search <media id>        Testing search for id, default is 'Q:0'\n"
        "  --debug                    Enable debug output\n"
        "  --help                     print this help\n"
        "\n", cmd
        );
}

static int g_loglevel = 1;

int main(int argc, char** argv)
{
  int ret = 0;
#ifdef __WINDOWS__
  //Initialize Winsock
  WSADATA wsaData;
  if ((ret = WSAStartup(MAKEWORD(2, 2), &wsaData)))
    return ret;
#endif /* __WINDOWS__ */

  std::string tryzone;
  std::string search = "Q:0";

  int i = 0;
  while (++i < argc)
  {
    if (strcmp(argv[i], "--debug") == 0)
    {
      g_loglevel = 4;
      PERROR("debug=Yes, ");
    }
    else if (strcmp(argv[i], "--zone") == 0 && argc > i+1)
    {
      PERRORF("zone=%s, ", argv[i+1]);
      tryzone.assign(argv[i+1]);
    }
    else if (strcmp(argv[i], "--search") == 0 && argc > i+1)
    {
      PERRORF("search=%s, ", argv[i+1]);
      search.assign(argv[i+1]);
    }
    else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
    {
      usage(argv[0]);
      return 0;
    }
  }
  PERROR("\n");
  SONOS::System::Debug(g_loglevel);

  {

    SONOS::System sonos(0, handleEventCB);
    if (sonos.Discover())
    {
      PRINT("Discovered !!!\n");
      SONOS::ZonePtr myZone;
      /*
       * Print Zones list and connect to
       */
      SONOS::ZoneList zones = sonos.GetZoneList();
      for (SONOS::ZoneList::const_iterator it = zones.begin(); it != zones.end(); ++it)
      {
        PERRORF("found zone '%s' with coordinator '%s'\n", it->second->GetZoneName().c_str(), it->second->GetCoordinator()->c_str());
        if (tryzone.empty())
          tryzone.assign(it->second->GetZoneName());
        if (it->second->GetZoneName() == tryzone)
          myZone = it->second;
      }

      /*
       * Print player infos
       */
      SONOS::ZonePlayerList players = sonos.GetZonePlayerList();
      for (SONOS::ZonePlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
      {
        PRINTF("found player '%s' at location %s [%s]\n", it->second->c_str(), it->second->GetLocation().c_str(),
                it->second->GetIconName().c_str());
      }

      if (sonos.IsConnected())
      {
        SONOS::PlayerPtr playerPtr = sonos.GetPlayer(myZone, 0, handleEventCB);

        SONOS::ElementList vars;
        playerPtr->GetTransportInfo(vars);
        for(unsigned i = 0; i < vars.size(); ++i)
          PRINTF("TransportInfo: %s : %s\n", vars[i]->GetKey().c_str(), vars[i]->c_str());

        SONOS::ContentDirectory mycontent(playerPtr->GetHost(), playerPtr->GetPort());
        SONOS::ContentList bdir(mycontent, search);
        PRINTF("UpdateID: %u\n", bdir.GetUpdateID());
        PRINTF("Item count: %u\n", bdir.size());
        SONOS::ContentList::iterator it = bdir.begin();
        int i = 0;
        while (it != bdir.end())
        {
          PRINTF("Item %d: [%d] [%s] [%s]\n", ++i, (*it)->IsItem(), (*it)->GetValue("dc:title").c_str(), (*it)->GetObjectID().c_str());
          if ((*it)->GetProperty("res"))
	    PRINTF("     %d: %s, %s\n", i, (*it)->GetValue("res").c_str(), (*it)->GetProperty("res")->GetAttribut("protocolInfo").c_str());
          SONOS::DigitalItemPtr payload;
          if (SONOS::System::ExtractObjectFromFavorite(*it, payload))
            PRINTF("   F %d: %s\n", i, payload->GetObjectID().c_str());
          else
            PRINTF("   I %d: %s\n", i, sonos.GetObjectIDFromUriMetadata(*it).c_str());
          ++it;
        }

        /*
         * Using class ContentBrowser to browse content ...
         *
        SONOS::ContentBrowser bdir2(mycontent, search);
        PRINT1("Item count: %u\n", bdir2.total());
        unsigned s = bdir2.index();
        while (s < bdir2.total() && bdir2.Browse(s, 100))
        {
          for (unsigned i = 0; i < bdir2.count(); ++i)
            PRINTF("Item %d: [%d] [%s]\n", i, bdir2.table()[i]->IsItem(), bdir2.table()[i]->GetValue("dc:title").c_str());
          s += bdir2.count();
        }
        */

        /*
         * Household ID
         */
        {
          std::string hhid = sonos.GetHouseholdID();
          PRINTF("Sonos Household ID = '%s'\n", hhid.c_str());
        }

      }
    }
  }

  //out:
#ifdef __WINDOWS__
  WSACleanup();
#endif /* __WINDOWS__ */
  return ret;
}
