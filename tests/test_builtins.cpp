#include <iostream>
#include <string>
#include <errno.h>

#include <test.h>
//#include <hashvalue.c>

#include <private/builtin.h>

TEST_CASE("string_to_int64")
{
  int64_t num;
  REQUIRE( string_to_int64("9223372036854775807", &num) == 0 );
  REQUIRE( num == 9223372036854775807LL );
  REQUIRE( string_to_int64("-9223372036854775807", &num) == 0 );
  REQUIRE( num == -9223372036854775807LL );
  REQUIRE( string_to_int64("-123.456789", &num) == -(EINVAL) );
}

TEST_CASE("string_to_int32")
{
  int32_t num;
  REQUIRE( string_to_int32("2147483647", &num) == 0 );
  REQUIRE( num == 2147483647L );
  REQUIRE( string_to_int32("-2147483647", &num) == 0 );
  REQUIRE( num == -2147483647L );
  REQUIRE( string_to_int32("-123.456789", &num) == -(EINVAL) );
  REQUIRE( string_to_int32("4294967296", &num) == -(ERANGE) );
}

TEST_CASE("string_to_int16")
{
  int16_t num;
  REQUIRE( string_to_int16("32767", &num) == 0 );
  REQUIRE( num == 32767 );
  REQUIRE( string_to_int16("-32767", &num) == 0 );
  REQUIRE( num == -32767 );
  REQUIRE( string_to_int16("-123.456789", &num) == -(EINVAL) );
  REQUIRE( string_to_int16("65536", &num) == -(ERANGE) );
}

TEST_CASE("string_to_int8")
{
  int8_t num;
  REQUIRE( string_to_int8("127", &num) == 0 );
  REQUIRE( num == 127 );
  REQUIRE( string_to_int8("-127", &num) == 0 );
  REQUIRE( num == -127 );
  REQUIRE( string_to_int8("-123.456789", &num) == -(EINVAL) );
  REQUIRE( string_to_int8("256", &num) == -(ERANGE) );
}

TEST_CASE("string_to_uint32")
{
  uint32_t num;
  REQUIRE( string_to_uint32("4294967295", &num) == 0 );
  REQUIRE( num == 4294967295L );
  REQUIRE( string_to_uint32("-2147483647", &num) == -(EINVAL) );
  REQUIRE( string_to_uint32("-123.456789", &num) == -(EINVAL) );
  REQUIRE( string_to_uint32("4294967296", &num) == -(ERANGE) );
}

TEST_CASE("string_to_uint16")
{
  uint16_t num;
  REQUIRE( string_to_uint16("65535", &num) == 0 );
  REQUIRE( num == 65535 );
  REQUIRE( string_to_uint16("-32767", &num) == -(EINVAL) );
  REQUIRE( string_to_uint16("-123.456789", &num) == -(EINVAL) );
  REQUIRE( string_to_uint16("65536", &num) == -(ERANGE) );
}

TEST_CASE("string_to_uint8")
{
  uint8_t num;
  REQUIRE( string_to_uint8("255", &num) == 0 );
  REQUIRE( num == 255 );
  REQUIRE( string_to_uint8("-127", &num) == -(EINVAL) );
  REQUIRE( string_to_uint8("-123.456789", &num) == -(EINVAL) );
  REQUIRE( string_to_uint8("256", &num) == -(ERANGE) );
}

TEST_CASE("string_to_double")
{
  double num;
  REQUIRE( string_to_double("123.456789", &num) == 0 );
  REQUIRE( num == 123.456789 );
  REQUIRE( string_to_double("-123.456789", &num) == 0 );
  REQUIRE( num == -123.456789 );
  REQUIRE( string_to_double("-1.23456789e+2", &num) == 0 );
  REQUIRE( num == -123.456789 );
  REQUIRE( string_to_double("+abcdef", &num) == -(EINVAL) );
}

TEST_CASE("string_to_timeout")
{
  unsigned tms;
  REQUIRE( string_to_timeout("40ms", &tms) == 0 );
  REQUIRE( tms == 40 );
  REQUIRE( string_to_timeout("40s", &tms) == 0 );
  REQUIRE( tms == 40000 );
  REQUIRE( string_to_timeout("40m", &tms) == 0 );
  REQUIRE( tms == 2400000 );
  REQUIRE( string_to_timeout("40h", &tms) == 0 );
  REQUIRE( tms == 144000000 );
  REQUIRE( string_to_timeout("40d", &tms) == 0 );
  REQUIRE( tms == 3456000000 );
  REQUIRE( string_to_timeout("1.5h", &tms) == 0 );
  REQUIRE( tms == 5400000 );
  REQUIRE( string_to_timeout("40", &tms) == -(EINVAL) );
  REQUIRE( string_to_timeout("4w", &tms) == -(EINVAL) );
}

TEST_CASE("string_to_size")
{
  int64_t sz;
  REQUIRE( string_to_size("40", &sz) == 0 );
  REQUIRE( sz == 40LL );
  REQUIRE( string_to_size("40k", &sz) == 0 );
  REQUIRE( sz == 40960LL );
  REQUIRE( string_to_size("40m", &sz) == 0 );
  REQUIRE( sz == 41943040LL );
  REQUIRE( string_to_size("40g", &sz) == 0 );
  REQUIRE( sz == 42949672960LL );
  REQUIRE( string_to_size("40b", &sz) == -(EINVAL) );
}

TEST_CASE("hex_to_num")
{
  int num;
  REQUIRE( hex_to_num("01020304", &num) == 0 );
  REQUIRE( num == 0x01020304 );
  REQUIRE( hex_to_num("05060708", &num) == 0 );
  REQUIRE( num == 0x05060708 );
  REQUIRE( hex_to_num("090a0b0c", &num) == 0 );
  REQUIRE( num == 0x090a0b0c );
  REQUIRE( hex_to_num("0d0e0f00", &num) == 0 );
  REQUIRE( num == 0x0d0e0f00 );
  REQUIRE( hex_to_num("7fffffff", &num) == 0 );
  REQUIRE( num == 0x7fffffff );
  REQUIRE( hex_to_num("80ABCDEF", &num) == 0 );
  REQUIRE( num == int(0x80ABCDEF) );
  REQUIRE( hex_to_num("FFFFFFFF", &num) == 0 );
  REQUIRE( num == int(0xffffffff) );
  REQUIRE( hex_to_num("", &num) == 0 );
  REQUIRE( num == 0 );
}

TEST_CASE("char_to_hex")
{
  BUILTIN_BUFFER buf;
  char_to_hex('\377', &buf);
  REQUIRE( std::string(buf.data) == "ff" );
  char_to_hex('\0', &buf);
  REQUIRE( std::string(buf.data) == "00" );
  char_to_hex('\177', &buf);
  REQUIRE( std::string(buf.data) == "7f" );
  char_to_hex('\200', &buf);
  REQUIRE( std::string(buf.data) == "80" );
  char_to_hex('\303', &buf);
  REQUIRE( std::string(buf.data) == "c3" );
}

TEST_CASE("char_to_uhex")
{
  BUILTIN_BUFFER buf;
  char_to_uhex('\377', &buf);
  REQUIRE( std::string(buf.data) == "FF" );
  char_to_uhex('\0', &buf);
  REQUIRE( std::string(buf.data) == "00" );
  char_to_uhex('\177', &buf);
  REQUIRE( std::string(buf.data) == "7F" );
  char_to_uhex('\200', &buf);
  REQUIRE( std::string(buf.data) == "80" );
  char_to_uhex('\303', &buf);
  REQUIRE( std::string(buf.data) == "C3" );
}

TEST_CASE("uint_to_strdec")
{
  BUILTIN_BUFFER buf;
  unsigned len;

  len = uint_to_strdec(255, buf.data, 10, 1);
  REQUIRE( len == 10 );
  REQUIRE( std::string(buf.data, len) == "0000000255" );

  len = uint_to_strdec(255, buf.data, 32, 0);
  REQUIRE( len == 3 );
  REQUIRE( std::string(buf.data, len) == "255" );

  len = uint_to_strdec(0, buf.data, 10, 1);
  REQUIRE( len == 10 );
  REQUIRE( std::string(buf.data, len) == "0000000000" );
}

TEST_CASE("uint64_to_strdec")
{
  BUILTIN_BUFFER buf;
  uint64_t len;

  len = uint64_to_strdec(9223372036854775807LL, buf.data, 22, 1);
  REQUIRE( len == 22 );
  REQUIRE( std::string(buf.data, len) == "0009223372036854775807" );

  len = uint64_to_strdec(9223372036854775807LL, buf.data, 32, 0);
  REQUIRE( len == 19 );
  REQUIRE( std::string(buf.data, len) == "9223372036854775807" );

  len = uint64_to_strdec(0LL, buf.data, 22, 1);
  REQUIRE( len == 22 );
  REQUIRE( std::string(buf.data, len) == "0000000000000000000000" );
}

TEST_CASE("string_to_time")
{
  time_t t0;
  REQUIRE( string_to_time("2012-07-31T13:09:27Z", &t0) == 0 );
  REQUIRE( t0 == time_t(1343740167L) );

  time_t t1, t2;
  REQUIRE( string_to_time("2012-07-31T00:00:00Z", &t1) == 0 );
  REQUIRE( string_to_time("2012-07-31T00:00:00", &t2) == 0 );
  double z = ::difftime(t1, t2);

  time_t t;
  REQUIRE( string_to_time("2012-07-31T13:09:27", &t) == 0 );
  REQUIRE( t == time_t(1343740167L - z) );

  REQUIRE( string_to_time("2012-07-31", &t) == 0 );
  REQUIRE( t == t2 );
}

TEST_CASE("time_to_iso8601utc")
{
  BUILTIN_BUFFER buf;
  time_t t0 = time_t(1343740167L);
  time_to_iso8601utc(t0, &buf);
  REQUIRE( std::string(buf.data) == "2012-07-31T13:09:27Z" );
}

TEST_CASE("time_to_iso8601")
{
  BUILTIN_BUFFER buf;
  time_t t0 = time_t(1343740167L);
  time_to_iso8601(t0, &buf);

  time_t t;
  REQUIRE( string_to_time(buf.data, &t) == 0 );
  REQUIRE( t == t0 );
}

TEST_CASE("time_to_isodate")
{
  BUILTIN_BUFFER buf;
  time_t t0 = time_t(1343736000L);
  time_to_isodate(t0, &buf);
  REQUIRE( std::string(buf.data) == "2012-07-31" );
}

TEST_CASE("time_to_httptime")
{
  BUILTIN_BUFFER buf;
  time_t t0 = time_t(1343736000L);
  time_to_httptime(t0, &buf);
  REQUIRE( std::string(buf.data) == "Tue, 31 Jul 2012 12:00:00 GMT" );
}

TEST_CASE("time_tz")
{
  tz_t tz;
  (void) time_tz(time_t(1343736000L), &tz);

  time_t t1, t2;
  REQUIRE( string_to_time("2012-07-31T00:00:00Z", &t1) == 0 );
  REQUIRE( string_to_time("2012-07-31T00:00:00", &t2) == 0 );
  double z = ::difftime(t1, t2);

  REQUIRE( (tz.tz_dir * (tz.tz_hour*3600 + tz.tz_min*60)) == int(z) );
}
