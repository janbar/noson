#include <iostream>
#include <list>

#include "include/testmain.h"
#include "include/soap_response_1.c"
#include "include/hashvalue.c"

#include <noson/element.h>
#include <noson/didlparser.h>
#include <private/tinyxml2.h>
#include <private/xmldict.h>

TEST_CASE("Parse SOAP response 1")
{
  const std::string data((const char*)soap_response_1_html, soap_response_1_html_len);
  SONOS::ElementList vars;

    // Parse xml content
  tinyxml2::XMLDocument rootdoc;
  REQUIRE((rootdoc.Parse(data.c_str(), data.size()) == tinyxml2::XML_SUCCESS));

  const tinyxml2::XMLElement* elem; // an element
  // Check for response: Envelope/Body/{respTag}

  REQUIRE(((elem = rootdoc.RootElement()) && SONOS::XMLNS::NameEqual(elem->Name(), "Envelope")));

  // search the element 'Body'
  elem = elem->FirstChildElement();
  while (elem && !SONOS::XMLNS::NameEqual(elem->Name(), "Body"))
    elem = elem->NextSiblingElement(nullptr);

  REQUIRE((elem && (elem = elem->FirstChildElement())));

  vars.push_back(SONOS::ElementPtr(new SONOS::Element("TAG", SONOS::XMLNS::LocalName(elem->Name()))));
  elem = elem->FirstChildElement(nullptr);
  while (elem)
  {
    if (elem->GetText())
    {
      // remove the namespace qualifier to handle local name as key
      vars.push_back(SONOS::ElementPtr(new SONOS::Element(SONOS::XMLNS::LocalName(elem->Name()), elem->GetText())));
    }
    elem = elem->NextSiblingElement(nullptr);
  }

  REQUIRE(vars.size() == 5);
  REQUIRE(vars[0]->GetKey() == "TAG");
  REQUIRE((*vars[0]) == "BrowseResponse");
  REQUIRE(vars[1]->GetKey() == "Result");
  REQUIRE((*vars[1]).substr(0, 10) == "<DIDL-Lite");
  REQUIRE(vars[2]->GetKey() == "NumberReturned");
  REQUIRE((*vars[2]) == "24");
  REQUIRE(vars[3]->GetKey() == "TotalMatches");
  REQUIRE((*vars[3]) == "24");
  REQUIRE(vars[4]->GetKey() == "UpdateID");
  REQUIRE((*vars[4]) == "154");

  // peer could return a valid result on out of range
  SONOS::DIDLParser didl(vars[1]->c_str());

  REQUIRE(didl.IsValid() == true);
  REQUIRE(didl.GetItems().size() == 24);

  SONOS::DigitalItemPtr item = didl.GetItems()[21];
  REQUIRE(item->GetValue("res") ==
          "x-file-cifs://bart/share/music/FLAC/Erik%20Satie/%c5%92uvres%20pour%"
          "20piano%20(France%20Clidat)/Embryons%20dess%c3%a9ch%c3%a9s%20_%20De%"
          "20Podophthalma.flac");
  REQUIRE(item->GetValue("upnp:albumArtURI") == 
          "/getaa?u=x-file-cifs%3a%2f%2fbart%2fshare%2fmusic%2fFLAC%2fErik%2520"
          "Satie%2f%25c5%2592uvres%2520pour%2520piano%2520(France%2520Clidat)%2"
          "fEmbryons%2520dess%25c3%25a9ch%25c3%25a9s%2520_%2520De%2520Podophtha"
          "lma.flac&v=291");
  REQUIRE(item->GetValue("dc:title") ==
          "Embryons desséchés : De Podophthalma");
  REQUIRE(item->GetValue("upnp:class") ==
          "object.item.audioItem.musicTrack");
  REQUIRE(item->GetValue("dc:creator") ==
          "Erik Satie");
  REQUIRE(item->GetValue("upnp:album") ==
          "Œuvres pour piano (France Clidat)");
  REQUIRE(item->GetValue("upnp:originalTrackNumber") ==
          "22");

  REQUIRE(item->DIDL() ==
          "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" xm"
          "lns:r=\"urn:schemas-rinconnetworks-com:metadata-1-0/\" xmlns:dc=\"ht"
          "tp://purl.org/dc/elements/1.1/\" xmlns:upnp=\"urn:schemas-upnp-org:m"
          "etadata-1-0/upnp/\" ><item id=\"Q:0/22\" parentID=\"Q:0\" restricted"
          "=\"true\"><res protocolInfo=\"x-file-cifs:*:audio/flac:*\">x-file-ci"
          "fs://bart/share/music/FLAC/Erik%20Satie/%c5%92uvres%20pour%20piano%2"
          "0(France%20Clidat)/Embryons%20dess%c3%a9ch%c3%a9s%20_%20De%20Podopht"
          "halma.flac</res><upnp:albumArtURI>/getaa?u=x-file-cifs%3a%2f%2fbart%"
          "2fshare%2fmusic%2fFLAC%2fErik%2520Satie%2f%25c5%2592uvres%2520pour%2"
          "520piano%2520(France%2520Clidat)%2fEmbryons%2520dess%25c3%25a9ch%25c"
          "3%25a9s%2520_%2520De%2520Podophthalma.flac&amp;v=291</upnp:albumArtU"
          "RI><dc:title>Embryons desséchés : De Podophthalma</dc:title><upnp:cl"
          "ass>object.item.audioItem.musicTrack</upnp:class><dc:creator>Erik Sa"
          "tie</dc:creator><upnp:album>Œuvres pour piano (France Clidat)</upnp:"
          "album><upnp:originalTrackNumber>22</upnp:originalTrackNumber></item>"
          "</DIDL-Lite>");

  SONOS::DIDLParser didl2(item->DIDL().c_str());
  REQUIRE(didl2.IsValid() == true);
}
