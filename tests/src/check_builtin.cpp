#include <iostream>

#include "include/testmain.h"

#include <private/builtin.h>
#include <string.h>
#include <string>

TEST_CASE("Decimal converter")
{
  BUILTIN_BUFFER buf;
  memset(buf.data, '\0', sizeof(buf.data));

  uint_to_strdec(1234567890, buf.data, 10, 0);
  REQUIRE((strlen(buf.data) == 10));
  std::cout << "DEC NO PADDING = " << std::string(buf.data) << std::endl;
  REQUIRE((strncmp(buf.data, "1234567890", 10) == 0));

  uint_to_strdec(987654321, buf.data, 16, 1);
  REQUIRE((strlen(buf.data) == 16));
  std::cout << "DEC PADDING 16 = " << std::string(buf.data) << std::endl;
  REQUIRE((strncmp(buf.data, "0000000987654321", 16) == 0));
}

TEST_CASE("Time converters")
{
  int r = 0;
  time_t now = time(0);
  time_t chk;

  BUILTIN_BUFFER buf;
  time_to_iso8601(now, &buf);
  r = string_to_time(buf.data, &chk);
  REQUIRE(r == 0);
  std::cout << "ISO8601 LOC = " << std::string(buf.data) << std::endl;
  REQUIRE(chk == now);

  BUILTIN_BUFFER buf2;
  time_to_isodate(now, &buf2);
  REQUIRE((strlen(buf2.data) == 10));
  std::cout << "ISODATE LOC = " << std::string(buf2.data) << std::endl;
  REQUIRE((strncmp(buf2.data, buf.data, 10) == 0));

  time_to_iso8601utc(now, &buf);
  r = string_to_time(buf.data, &chk);
  REQUIRE(r == 0);
  std::cout << "ISO8601 UTC = " << std::string(buf.data) << std::endl;
  REQUIRE(chk == now);
}

