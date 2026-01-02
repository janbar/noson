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

#include "local_config.h"

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

typedef struct { unsigned sz; const char* txt; } WS_METHOD_TABLE_t;
extern const WS_METHOD_TABLE_t ws_method_table[];
extern WS_METHOD ws_method_from_str(const char* str);
CC_INLINE const char* ws_method_to_str(WS_METHOD m) { return ws_method_table[(unsigned)m].txt; }

typedef enum
{
  WS_CTYPE_None     = 0,
  WS_CTYPE_Form     = 1,
  WS_CTYPE_Raw,
  WS_CTYPE_Any,
  WS_CTYPE_UNKNOWN,
} WS_CTYPE;

typedef struct { unsigned sz; const char* txt; } WS_CTYPE_TABLE_t;
extern const WS_CTYPE_TABLE_t ws_ctype_table[];
extern WS_CTYPE ws_ctype_from_str(const char* str);
CC_INLINE const char* ws_ctype_to_str(WS_CTYPE t) { return ws_ctype_table[(unsigned)t].txt; }

typedef enum
{
  WS_CENCODING_None     = 0,
  WS_CENCODING_Deflate  = 1,
  WS_CENCODING_Gzip,
  WS_CENCODING_UNKNOWN,
} WS_CENCODING;

typedef struct { unsigned sz; const char* txt; } WS_CENCODING_TABLE_t;
extern const WS_CENCODING_TABLE_t ws_cencoding_table[];
extern WS_CENCODING ws_cencoding_from_str(const char* str);
CC_INLINE const char* ws_cencoding_to_str(WS_CENCODING e) { return ws_cencoding_table[(unsigned)e].txt; }

typedef enum
{
  WS_HEADER_Accept          = 0,
  WS_HEADER_Accept_Charset  = 1,
  WS_HEADER_Accept_Encoding,
  WS_HEADER_Connection,
  WS_HEADER_Content_Encoding,
  WS_HEADER_Content_Length,
  WS_HEADER_Content_Range,
  WS_HEADER_Content_Type,
  WS_HEADER_Etag,
  WS_HEADER_Host,
  WS_HEADER_Keep_ALive,
  WS_HEADER_Location,
  WS_HEADER_Server,
  WS_HEADER_Transfer_Encoding,
  WS_HEADER_User_Agent,
  WS_HEADER_UNKNOWN,
} WS_HEADER;

typedef struct { unsigned sz; const char* txt; const char* upper_txt; } WS_HEADER_TABLE_t;
extern const WS_HEADER_TABLE_t ws_header_table[];
extern WS_HEADER ws_header_from_upperstr(const char* upperstr);
CC_INLINE const char* ws_header_to_str(WS_HEADER h) { return ws_header_table[(unsigned)h].txt; }
CC_INLINE const char* ws_header_to_upperstr(WS_HEADER h) { return ws_header_table[(unsigned)h].upper_txt; }

typedef enum
{
  /* 2xx */
  WS_STATUS_OK        = 0,
  WS_STATUS_Created   = 1,
  WS_STATUS_Accepted,
  WS_STATUS_Non_Authoritative_Information,
  WS_STATUS_No_Content,
  WS_STATUS_Reset_Content,
  WS_STATUS_Partial_Content,

  /* 3xx */
  WS_STATUS_Moved_Permanently,
  WS_STATUS_Moved_Temporarily,
  WS_STATUS_See_Other,
  WS_STATUS_Not_modified,
  WS_STATUS_Use_Proxy,
  WS_STATUS_Temporary_Redirect,
  WS_STATUS_Permanent_Redirect,

  /* 4xx */
  WS_STATUS_Bad_Request,
  WS_STATUS_Unauthorized,
  WS_STATUS_Payment_Required,
  WS_STATUS_Forbidden,
  WS_STATUS_Not_Found,
  WS_STATUS_Method_Not_Allowed,
  WS_STATUS_Not_Acceptable,
  WS_STATUS_Proxy_Authentication_Required,
  WS_STATUS_Request_Timeout,
  WS_STATUS_Conflict,
  WS_STATUS_Gone,
  WS_STATUS_Length_Required,
  WS_STATUS_Precondition_Failed,
  WS_STATUS_Content_Too_Large,
  WS_STATUS_URI_Too_Long,
  WS_STATUS_Unsupported_Media_Type,
  WS_STATUS_Range_Not_Satisfiable,
  WS_STATUS_Expectation_Failed,
  WS_STATUS_I_m_a_teapot,
  WS_STATUS_Misdirected_Request,
  WS_STATUS_Upgrade_Required,
  WS_STATUS_Precondition_Required,
  WS_STATUS_Too_Many_Requests,
  WS_STATUS_Request_Header_Fields_Too_Large,
  WS_STATUS_Unavailable_For_Legal_Reasons,

  /* 5xx */
  WS_STATUS_Internal_Server_Error,
  WS_STATUS_Not_Implemented,
  WS_STATUS_Bad_Gateway,
  WS_STATUS_Service_Unavailable,
  WS_STATUS_Gateway_Timeout,
  WS_STATUS_HTTP_Version_Not_Supported,
  WS_STATUS_Variant_Also_Negotiates,
  WS_STATUS_Not_Extended,
  WS_STATUS_Network_Authentication_Required,

  /* 1xx */
  WS_STATUS_Continue,
  WS_STATUS_Switching_Protocols,

  WS_STATUS_UNKNOWN,
} WS_STATUS;

typedef struct { unsigned sz; const char* numstr; const char* msgstr; int num; } WS_STATUS_TABLE_t;
extern const WS_STATUS_TABLE_t ws_status_table[];
extern WS_STATUS ws_status_from_num(int num);
extern WS_STATUS ws_status_from_numstr(const char* numstr);
CC_INLINE int ws_status_to_num(WS_STATUS s) { return ws_status_table[(unsigned)s].num; }
CC_INLINE const char* ws_status_to_numstr(WS_STATUS s) { return ws_status_table[(unsigned)s].numstr; }
CC_INLINE const char* ws_status_to_msgstr(WS_STATUS s) { return ws_status_table[(unsigned)s].msgstr; }

#ifdef __cplusplus
}
#endif

#endif	/* WSSTATIC_H */
