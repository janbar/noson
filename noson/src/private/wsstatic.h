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

#ifndef WSSTATIC_H
#define	WSSTATIC_H

#if defined __cplusplus
extern "C" {
#endif

#define WS_CRLF       "\r\n"
#define WS_CRLF_LEN   2

typedef enum
{
  WS_METHOD_Get     = 0,
  WS_METHOD_Post    = 1,
  WS_METHOD_Head,
  WS_METHOD_Subscribe,
  WS_METHOD_Unsubscribe,
  WS_METHOD_Notify,
  WS_METHOD_Put,
  WS_METHOD_Delete,
  WS_METHOD_Options,
  WS_METHOD_UNKNOWN,
} WS_METHOD;

extern WS_METHOD ws_method_from_str(const char* str);
extern const char* ws_method_to_str(WS_METHOD m);

typedef enum
{
  WS_CTYPE_None     = 0,
  WS_CTYPE_Form     = 1,
  WS_CTYPE_Raw,
  WS_CTYPE_Any,
  WS_CTYPE_UNKNOWN,
} WS_CTYPE;

extern WS_CTYPE ws_ctype_from_str(const char* str);
extern const char* ws_ctype_to_str(WS_CTYPE t);

typedef enum
{
  WS_CENCODING_None     = 0,
  WS_CENCODING_Deflate  = 1,
  WS_CENCODING_Gzip,
  WS_CENCODING_UNKNOWN,
} WS_CENCODING;

extern WS_CENCODING ws_cencoding_from_str(const char* str);
extern const char* ws_cencoding_to_str(WS_CENCODING e);

typedef enum
{
  WS_HEADER_Accept          = 0,
  WS_HEADER_Accept_Charset  = 1,
  WS_HEADER_Accept_Encoding,
  WS_HEADER_Accept_Language,
  WS_HEADER_Accept_Ranges,
  WS_HEADER_Authorization,
  WS_HEADER_Cache_Control,
  WS_HEADER_Connection,
  WS_HEADER_Content_Encoding,
  WS_HEADER_Content_Length,
  WS_HEADER_Content_Range,
  WS_HEADER_Content_Type,
  WS_HEADER_ETag,
  WS_HEADER_Expires,
  WS_HEADER_Host,
  WS_HEADER_If_Match,
  WS_HEADER_If_None_Match,
  WS_HEADER_Keep_ALive,
  WS_HEADER_Last_Modified,
  WS_HEADER_Location,
  WS_HEADER_Range,
  WS_HEADER_Server,
  WS_HEADER_Transfer_Encoding,
  WS_HEADER_User_Agent,
  WS_HEADER_WWW_Authenticate,
  WS_HEADER_UNKNOWN,
} WS_HEADER;

extern WS_HEADER ws_header_from_upperstr(const char* upperstr);
extern const char* ws_header_to_str(WS_HEADER h);
extern const char* ws_header_to_upperstr(WS_HEADER h);

typedef enum
{
  /* 2xx */
  WS_STATUS_200_OK        = 0,
  WS_STATUS_201_Created   = 1,
  WS_STATUS_202_Accepted,
  WS_STATUS_203_Non_Authoritative,
  WS_STATUS_204_No_Content,
  WS_STATUS_205_Reset_Content,
  WS_STATUS_206_Partial_Content,

  /* 3xx */
  WS_STATUS_301_Moved_Permanently,
  WS_STATUS_302_Moved_Temporarily,
  WS_STATUS_303_See_Other,
  WS_STATUS_304_Not_modified,
  WS_STATUS_305_Use_Proxy,
  WS_STATUS_307_Temporary_Redirect,
  WS_STATUS_308_Permanent_Redirect,

  /* 4xx */
  WS_STATUS_400_Bad_Request,
  WS_STATUS_401_Unauthorized,
  WS_STATUS_402_Payment_Required,
  WS_STATUS_403_Forbidden,
  WS_STATUS_404_Not_Found,
  WS_STATUS_405_Method_Not_Allowed,
  WS_STATUS_406_Not_Acceptable,
  WS_STATUS_407_Proxy_Authent_Required,
  WS_STATUS_408_Request_Timeout,
  WS_STATUS_409_Conflict,
  WS_STATUS_410_Gone,
  WS_STATUS_411_Length_Required,
  WS_STATUS_412_Precondition_Failed,
  WS_STATUS_413_Content_Too_Large,
  WS_STATUS_414_URI_Too_Long,
  WS_STATUS_415_Unsupported_Media_Type,
  WS_STATUS_416_Range_Not_Satisfiable,
  WS_STATUS_417_Expectation_Failed,
  WS_STATUS_418_I_m_a_teapot,
  WS_STATUS_421_Misdirected_Request,
  WS_STATUS_426_Upgrade_Required,
  WS_STATUS_428_Precondition_Required,
  WS_STATUS_429_Too_Many_Requests,
  WS_STATUS_431_Request_Header_Too_Large,
  WS_STATUS_451_Unavailable_For_LR,

  /* 5xx */
  WS_STATUS_500_Internal_Server_Error,
  WS_STATUS_501_Not_Implemented,
  WS_STATUS_502_Bad_Gateway,
  WS_STATUS_503_Service_Unavailable,
  WS_STATUS_504_Gateway_Timeout,
  WS_STATUS_505_HTTP_Ver_Not_Supported,
  WS_STATUS_506_Variant_Also_Negotiates,
  WS_STATUS_510_Not_Extended,
  WS_STATUS_511_Network_Authent_Required,

  /* 1xx */
  WS_STATUS_100_Continue,
  WS_STATUS_101_Switching_Protocols,

  WS_STATUS_UNKNOWN,
} WS_STATUS;

extern WS_STATUS ws_status_from_num(int num);
extern WS_STATUS ws_status_from_numstr(const char* numstr);
extern int ws_status_to_num(WS_STATUS s);
extern const char* ws_status_to_numstr(WS_STATUS s);
extern const char* ws_status_to_msgstr(WS_STATUS s);

#ifdef __cplusplus
}
#endif

#endif	/* WSSTATIC_H */
