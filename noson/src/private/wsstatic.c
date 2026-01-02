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

const WS_METHOD_TABLE_t ws_method_table[] = {
  { 4,  "GET" },
  { 5,  "POST" },
  { 5,  "HEAD" },
  { 10, "SUBSCRIBE" },
  { 12, "UNSUBSCRIBE" },
  { 7,  "NOTIFY" },
  { 4,  "PUT" },
  { 7,  "DELETE" },
  { 8,  "OPTIONS" },
  { 0,  NULL }
};

WS_METHOD ws_method_from_str(const char* str)
{
  const WS_METHOD_TABLE_t* p = ws_method_table;
  while (p->txt)
  {
    if (memcmp(p->txt, str, p->sz) == 0)
      return (WS_METHOD)(p - ws_method_table);
    ++p;
  }
  return WS_METHOD_UNKNOWN;
}

const WS_CTYPE_TABLE_t ws_ctype_table[] = {
  { 1,  "" },
  { 34, "application/x-www-form-urlencoded" },
  { 25, "application/octet-stream" },
  { 4,  "*/*" },
  { 0,  NULL }
};

WS_CTYPE ws_ctype_from_str(const char* str)
{
  const WS_CTYPE_TABLE_t* p = ws_ctype_table;
  while (p->txt)
  {
    if (memcmp(p->txt, str, p->sz) == 0)
      return (WS_CTYPE)(p - ws_ctype_table);
    ++p;
  }
  return WS_CTYPE_UNKNOWN;
}

const WS_CENCODING_TABLE_t ws_cencoding_table[] = {
  { 1,  "" },
  { 8,  "deflate" },
  { 5,  "gzip" },
  { 0,  NULL }
};

WS_CENCODING ws_cencoding_from_str(const char* str)
{
  const WS_CENCODING_TABLE_t* p = ws_cencoding_table;
  while (p->txt)
  {
    if (memcmp(p->txt, str, p->sz) == 0)
      return (WS_CENCODING)(p - ws_cencoding_table);
    ++p;
  }
  return WS_CENCODING_UNKNOWN;
}

const WS_HEADER_TABLE_t ws_header_table[] = {
  { 7,  "Accept",                 "ACCEPT" },
  { 15, "Accept-Charset",         "ACCEPT-CHARSET" },
  { 16, "Accept-Encoding",        "ACCEPT-ENCODING" },
  { 11, "Connection",             "CONNECTION" },
  { 17, "Content-Encoding",       "CONTENT-ENCODING" },
  { 15, "Content-Length",         "CONTENT-LENGTH" },
  { 15, "Content-Range",          "CONTENT-RANGE" },
  { 13, "Content-Type",           "CONTENT-TYPE" },
  { 5,  "Etag",                   "ETAG" },
  { 5,  "Host",                   "HOST" },
  { 11, "Keep-Alive",             "KEEP-ALIVE" },
  { 9,  "Location",               "LOCATION" },
  { 7,  "Server",                 "SERVER" },
  { 18, "Transfer-Encoding",      "TRANSFER-ENCODING" },
  { 11, "User-Agent",             "USER-AGENT" },
  { 0,  NULL }
};

WS_HEADER ws_header_from_upperstr(const char* upperstr)
{
  const WS_HEADER_TABLE_t* p = ws_header_table;
  while (p->upper_txt)
  {
    if (memcmp(p->upper_txt, upperstr, p->sz) == 0)
      return (WS_HEADER)(p - ws_header_table);
    ++p;
  }
  return WS_HEADER_UNKNOWN;
}

const WS_STATUS_TABLE_t ws_status_table[] = {
  /* 2xx */
  { 4,  "200",  "OK",                                 200 },
  { 4,  "201",  "Created",                            201 },
  { 4,  "202",  "Accepted",                           202 },
  { 4,  "203",  "Non-Authoritative Information",      203 },
  { 4,  "204",  "No content",                         204 },
  { 4,  "205",  "Reset Content",                      205 },
  { 4,  "206",  "Partial content",                    206 },

  /* 3xx */
  { 4,  "301",  "Moved permanently",                  301 },
  { 4,  "302",  "Moved temporarily",                  302 },
  { 4,  "303",  "See Other",                          303 },
  { 4,  "304",  "Not modified",                       304 },
  { 4,  "305",  "Use Proxy",                          305 },
  { 4,  "307",  "Temporary Redirect",                 307 },
  { 4,  "308",  "Permanent Redirect",                 308 },

  /* 4xx */
  { 4,  "400",  "Bad request",                        400 },
  { 4,  "401",  "Unauthorized",                       401 },
  { 4,  "402",  "Payment Required",                   402 },
  { 4,  "403",  "Forbidden",                          403 },
  { 4,  "404",  "Not found",                          404 },
  { 4,  "405",  "Method Not Allowed",                 405 },
  { 4,  "406",  "Not Acceptable",                     406 },
  { 4,  "407",  "Proxy Authentication Required",      407 },
  { 4,  "408",  "Request Timeout",                    408 },
  { 4,  "409",  "Conflict",                           409 },
  { 4,  "410",  "Gone",                               410 },
  { 4,  "411",  "Length Required",                    411 },
  { 4,  "412",  "Precondition Failed",                412 },
  { 4,  "413",  "Content Too Large",                  413 },
  { 4,  "414",  "URI Too Long",                       414 },
  { 4,  "415",  "Unsupported Media Type",             415 },
  { 4,  "416",  "Range Not Satisfiable",              416 },
  { 4,  "417",  "Expectation Failed",                 417 },
  { 4,  "418",  "I'm a teapot",                       418 },
  { 4,  "421",  "Misdirected Request",                421 },
  { 4,  "426",  "Upgrade Required",                   426 },
  { 4,  "428",  "Precondition Required",              428 },
  { 4,  "429",  "Too Many Requests",                  429 },
  { 4,  "431",  "Request Header Fields Too Large",    431 },
  { 4,  "451",  "Unavailable For Legal Reasons",      451 },

  /* 5xx */
  { 4,  "500",  "Internal server error",              500 },
  { 4,  "501",  "Not implemented",                    501 },
  { 4,  "502",  "Bad gateway",                        502 },
  { 4,  "503",  "Service unavailable",                503 },
  { 4,  "504",  "Gateway Timeout",                    504 },
  { 4,  "505",  "HTTP Version Not Supported",         505 },
  { 4,  "506",  "Variant Also Negotiates",            506 },
  { 4,  "510",  "Not Extended",                       510 },
  { 4,  "511",  "Network Authentication Required",    511 },

  /* 1xx */
  { 4,  "100",  "Continue",                           100 },
  { 4,  "101",  "Switching Protocols",                101 },

  { 0,  NULL , NULL, 0 }
};


WS_STATUS ws_status_from_num(int num)
{
  const WS_STATUS_TABLE_t* p = ws_status_table;
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
  const WS_STATUS_TABLE_t* p = ws_status_table;
  while (p->numstr)
  {
    if (memcmp(p->numstr, numstr, p->sz) == 0)
      return (WS_STATUS)(p - ws_status_table);
    ++p;
  }
  return WS_STATUS_UNKNOWN;
}
