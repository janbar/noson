/*
 *      Copyright (C) 2014-2025 Jean-Luc Barriere
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "wsstatic.h"

#include <stddef.h>  // for NULL
#include <string.h>  // for memcmp

typedef struct { unsigned sz; const char* txt; } WS_METHOD_TABLE;
static const WS_METHOD_TABLE ws_method_table[] = {
  { 4,  "GET" },
  { 5,  "POST" },
  { 5,  "HEAD" },
  { 8,  "OPTIONS" },
  { 10, "SUBSCRIBE" },
  { 12, "UNSUBSCRIBE" },
  { 7,  "NOTIFY" },
  { 4,  "PUT" },
  { 7,  "DELETE" },
  { 6,  "PATCH" },
  { 0,  NULL }
};

WS_METHOD ws_method_from_str(const char* str)
{
  const WS_METHOD_TABLE* p = ws_method_table;
  while (p->txt)
  {
    if (memcmp(p->txt, str, p->sz) == 0)
      return (WS_METHOD)(p - ws_method_table);
    ++p;
  }
  return WS_METHOD_UNKNOWN;
}

const char* ws_method_to_str(WS_METHOD m)
{
  return ws_method_table[(unsigned)m].txt;
}

typedef struct { unsigned sz; const char* txt; } WS_CTYPE_TABLE;
static const WS_CTYPE_TABLE ws_ctype_table[] = {
  { 1,  "" },
  { 34, "application/x-www-form-urlencoded" },
  { 25, "application/octet-stream" },
  { 4,  "*/*" },
  { 0,  NULL }
};

WS_CTYPE ws_ctype_from_str(const char* str)
{
  const WS_CTYPE_TABLE* p = ws_ctype_table;
  while (p->txt)
  {
    if (memcmp(p->txt, str, p->sz) == 0)
      return (WS_CTYPE)(p - ws_ctype_table);
    ++p;
  }
  return WS_CTYPE_UNKNOWN;
}

const char* ws_ctype_to_str(WS_CTYPE t)
{
  return ws_ctype_table[(unsigned)t].txt;
}

typedef struct { unsigned sz; const char* txt; } WS_CENCODING_TABLE;
static const WS_CENCODING_TABLE ws_cencoding_table[] = {
  { 1,  "" },
  { 8,  "deflate" },
  { 5,  "gzip" },
  { 0,  NULL }
};

WS_CENCODING ws_cencoding_from_str(const char* str)
{
  const WS_CENCODING_TABLE* p = ws_cencoding_table;
  while (p->txt)
  {
    if (memcmp(p->txt, str, p->sz) == 0)
      return (WS_CENCODING)(p - ws_cencoding_table);
    ++p;
  }
  return WS_CENCODING_UNKNOWN;
}

const char* ws_cencoding_to_str(WS_CENCODING e)
{
  return ws_cencoding_table[(unsigned)e].txt;
}

typedef struct { unsigned sz; const char* txt; const char* upper_txt; } WS_HEADER_TABLE;
static const WS_HEADER_TABLE ws_header_table[] = {
  { 7,  "Accept",                 "ACCEPT" },
  { 15, "Accept-Charset",         "ACCEPT-CHARSET" },
  { 16, "Accept-Encoding",        "ACCEPT-ENCODING" },
  { 16, "Accept-Language",        "ACCEPT-LANGUAGE" },
  { 14, "Accept-Ranges",          "ACCEPT-RANGES" },
  { 14, "Authorization",          "AUTHORIZATION" },
  { 14, "Cache-Control",          "CACHE-CONTROL" },
  { 11, "Connection",             "CONNECTION" },
  { 17, "Content-Encoding",       "CONTENT-ENCODING" },
  { 15, "Content-Length",         "CONTENT-LENGTH" },
  { 17, "Content-Location",       "CONTENT-LOCATION" },
  { 15, "Content-Range",          "CONTENT-RANGE" },
  { 13, "Content-Type",           "CONTENT-TYPE" },
  { 5,  "ETag",                   "ETAG" },
  { 8,  "Expires",                "EXPIRES" },
  { 5,  "Host",                   "HOST" },
  { 9,  "If-Match",               "IF-MATCH" },
  { 14, "If-None-Match",          "IF-NONE-MATCH" },
  { 11, "Keep-Alive",             "KEEP-ALIVE" },
  { 14, "Last-Modified",          "LAST-MODIFIED" },
  { 9,  "Location",               "LOCATION" },
  { 6,  "Range",                  "RANGE" },
  { 7,  "Server",                 "SERVER" },
  { 18, "Transfer-Encoding",      "TRANSFER-ENCODING" },
  { 11, "User-Agent",             "USER-AGENT" },
  { 17, "WWW-Authenticate",       "WWW-AUTHENTICATE" },
  { 16, "X-Forwarded-For",        "X-FORWARDED-FOR" },
  { 17, "X-Forwarded-Host",       "X-FORWARDED-HOST" },
  { 18, "X-Forwarded-Proto",      "X-FORWARDED-PROTO" },
  { 10, "X-Real-IP",              "X-REAL-IP" },
  { 0,  NULL, NULL }
};

WS_HEADER ws_header_from_upperstr(const char* upperstr)
{
  const WS_HEADER_TABLE* p = ws_header_table;
  while (p->upper_txt)
  {
    if (memcmp(p->upper_txt, upperstr, p->sz) == 0)
      return (WS_HEADER)(p - ws_header_table);
    ++p;
  }
  return WS_HEADER_UNKNOWN;
}

const char* ws_header_to_str(WS_HEADER h)
{
  return ws_header_table[(unsigned)h].txt;
}

const char* ws_header_to_upperstr(WS_HEADER h)
{
  return ws_header_table[(unsigned)h].upper_txt;
}

typedef struct { int num; unsigned sz; const char* numstr; const char* msgstr; WS_CLOSE close; } WS_STATUS_TABLE;
static const WS_STATUS_TABLE ws_status_table[] = {
  /* 2xx */
  { 200, 4,  "200",  "OK",                              WS_CLOSE_NO },
  { 201, 4,  "201",  "Created",                         WS_CLOSE_NO },
  { 202, 4,  "202",  "Accepted",                        WS_CLOSE_NO },
  { 203, 4,  "203",  "Non-Authoritative Information",   WS_CLOSE_NO },
  { 204, 4,  "204",  "No content",                      WS_CLOSE_NO },
  { 205, 4,  "205",  "Reset Content",                   WS_CLOSE_NO },
  { 206, 4,  "206",  "Partial content",                 WS_CLOSE_NO },
  { 207, 4,  "207",  "Multi-Status",                    WS_CLOSE_NO },
  { 208, 4,  "208",  "Already Reported",                WS_CLOSE_NO },

  /* 3xx */
  { 301, 4,  "301",  "Moved permanently",               WS_CLOSE_NO },
  { 302, 4,  "302",  "Moved temporarily",               WS_CLOSE_NO },
  { 303, 4,  "303",  "See Other",                       WS_CLOSE_NO },
  { 304, 4,  "304",  "Not modified",                    WS_CLOSE_NO },
  { 305, 4,  "305",  "Use Proxy",                       WS_CLOSE_NO },
  { 306, 4,  "306",  "RESERVED",                        WS_CLOSE_NO },
  { 307, 4,  "307",  "Temporary Redirect",              WS_CLOSE_NO },
  { 308, 4,  "308",  "Permanent Redirect",              WS_CLOSE_NO },

  /* 4xx */
  { 400, 4,  "400",  "Bad request",                     WS_CLOSE_YES },
  { 401, 4,  "401",  "Unauthorized",                    WS_CLOSE_NO },
  { 402, 4,  "402",  "Payment Required",                WS_CLOSE_YES },
  { 403, 4,  "403",  "Forbidden",                       WS_CLOSE_NO },
  { 404, 4,  "404",  "Not found",                       WS_CLOSE_NO },
  { 405, 4,  "405",  "Method Not Allowed",              WS_CLOSE_NO },
  { 406, 4,  "406",  "Not Acceptable",                  WS_CLOSE_NO },
  { 407, 4,  "407",  "Proxy Authentication Required",   WS_CLOSE_NO },
  { 408, 4,  "408",  "Request Timeout",                 WS_CLOSE_YES },
  { 409, 4,  "409",  "Conflict",                        WS_CLOSE_NO },
  { 410, 4,  "410",  "Gone",                            WS_CLOSE_NO },
  { 411, 4,  "411",  "Length Required",                 WS_CLOSE_YES },
  { 412, 4,  "412",  "Precondition Failed",             WS_CLOSE_NO },
  { 413, 4,  "413",  "Content Too Large",               WS_CLOSE_YES },
  { 414, 4,  "414",  "URI Too Long",                    WS_CLOSE_YES },
  { 415, 4,  "415",  "Unsupported Media Type",          WS_CLOSE_NO },
  { 416, 4,  "416",  "Range Not Satisfiable",           WS_CLOSE_NO },
  { 417, 4,  "417",  "Expectation Failed",              WS_CLOSE_NO },
  { 418, 4,  "418",  "I'm a teapot",                    WS_CLOSE_YES },

  { 421, 4,  "421",  "Misdirected Request",             WS_CLOSE_YES },
  { 422, 4,  "422",  "Unprocessable Content",           WS_CLOSE_YES },
  { 423, 4,  "423",  "Locked",                          WS_CLOSE_YES },
  { 424, 4,  "424",  "Failed Dependency",               WS_CLOSE_YES },
  { 425, 4,  "425",  "Too Early",                       WS_CLOSE_YES },
  { 426, 4,  "426",  "Upgrade Required",                WS_CLOSE_YES },

  { 428, 4,  "428",  "Precondition Required",           WS_CLOSE_YES },
  { 429, 4,  "429",  "Too Many Requests",               WS_CLOSE_YES },

  { 431, 4,  "431",  "Request Header Fields Too Large", WS_CLOSE_YES },

  { 451, 4,  "451",  "Unavailable For Legal Reasons",   WS_CLOSE_YES },

  /* 5xx */
  { 500, 4,  "500",  "Internal server error",           WS_CLOSE_YES },
  { 501, 4,  "501",  "Not implemented",                 WS_CLOSE_YES },
  { 502, 4,  "502",  "Bad gateway",                     WS_CLOSE_YES },
  { 503, 4,  "503",  "Service unavailable",             WS_CLOSE_YES },
  { 504, 4,  "504",  "Gateway Timeout",                 WS_CLOSE_YES },
  { 505, 4,  "505",  "HTTP Version Not Supported",      WS_CLOSE_YES },
  { 506, 4,  "506",  "Variant Also Negotiates",         WS_CLOSE_YES },
  { 507, 4,  "507",  "Insufficient Storage",            WS_CLOSE_YES },
  { 508, 4,  "508",  "Loop Detected",                   WS_CLOSE_YES },

  { 510, 4,  "510",  "Not Extended",                    WS_CLOSE_YES },
  { 511, 4,  "511",  "Network Authentication Required", WS_CLOSE_YES },

  /* 1xx */
  { 100, 4,  "100",  "Continue",                        WS_CLOSE_NO },
  { 101, 4,  "101",  "Switching Protocols",             WS_CLOSE_YES },
  { 102, 4,  "102",  "Processing",                      WS_CLOSE_NO },
  { 103, 4,  "103",  "Early Hints",                     WS_CLOSE_NO },

  { 0, 0,  NULL , NULL, WS_CLOSE_YES }
};


WS_STATUS ws_status_from_num(int num)
{
  const WS_STATUS_TABLE* p = ws_status_table;
  while (p->numstr)
  {
    if (p->num == num)
      return (WS_STATUS)(p - ws_status_table);
    ++p;
  }
  return WS_STATUS_UNKNOWN;
}

WS_STATUS ws_status_from_numstr(const char* numstr)
{
  const WS_STATUS_TABLE* p = ws_status_table;
  while (p->numstr)
  {
    if (memcmp(p->numstr, numstr, p->sz) == 0)
      return (WS_STATUS)(p - ws_status_table);
    ++p;
  }
  return WS_STATUS_UNKNOWN;
}

int ws_status_to_num(WS_STATUS s)
{
  return ws_status_table[(unsigned) s].num;
}

const char* ws_status_to_numstr(WS_STATUS s)
{
  return ws_status_table[(unsigned) s].numstr;
}

const char* ws_status_to_msgstr(WS_STATUS s)
{
  return ws_status_table[(unsigned) s].msgstr;
}

WS_CLOSE ws_status_to_close(WS_STATUS s)
{
  return ws_status_table[(unsigned) s].close;
}
