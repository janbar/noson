/*
 *      Copyright (C) 2026 Jean-Luc Barriere
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

#ifndef WSREQUESTREPLY_H
#define WSREQUESTREPLY_H

#include "local_config.h"
#include "wsrequestbroker.h"
#include "wsstatic.h"

#include <string>
#include <map>

namespace NSROOT
{

  class WSReplyChunked;
  class InputStream;

  class WSRequestReply
  {
  public:
    WSRequestReply(WSRequestBroker& rb);
    ~WSRequestReply();

    enum STAGE
    {
      STAGE_HEADER  = 0,
      STAGE_CONTENT = 1,
      STAGE_CLOSE   = 2,
    };

    STAGE GetStage() const { return m_stage; }

    void AddHeader(const std::string& headerStr, const std::string& str, bool encap = false);
    void AddHeader(WS_HEADER header, const std::string& str, bool encap = false);
    void AddHeader(WS_HEADER header, uint32_t num);

    /**
     * Clear the header to refill and post.
     * Note: Only valid at stage HEADER.
     * @return true on success, else false
     */
    bool ResetReply();

    /**
     * Post the header with the given status, and close reply.
     * Note: Only valid at stage HEADER.
     * @param status The reply status
     * @return true on success, else false
     */
    bool PostReply(WS_STATUS status);

    /**
     * Post the header with the given status, and initialize the content
     * stream. The stage move to CONTENT.
     * Note: Only valid at stage HEADER.
     * @param status The reply status
     * @return true on success, else false
     */
    bool BeginContent(WS_STATUS status, int chunkSize);

    /**
     * Write data in content stream.
     * Note: Only valid at stage CONTENT.
     * @param data
     * @param len The byte count to send
     * @return true on success, else false
     */
    bool WriteData(const char* data, int len);

    /**
     * Write FILE stream into the content stream.
     * Note: Only valid at stage CONTENT. It should be run in a loop
     * until the end of the FILE stream.
     * @param file
     * @return The number of bytes actually written
     */
    int WriteFileStream(FILE* file);

    /**
     * Write FILE stream into the content stream.
     * Note: Only valid at stage CONTENT. It should be run in a loop
     * until the desired number of bytes is obtained.
     * @param file
     * @param maxlen The maximum number of bytes to write
     * @return The number of bytes actually written
     */
    int WriteFileStream(FILE* file, unsigned maxlen);


    /**
     * Write imput stream into the content stream.
     * Note: Only valid at stage CONTENT. It should be run in a loop
     * until the end of the input stream.
     * @param in
     * @return The number of bytes actually written
     */
    int WriteInputStream(InputStream& in);

    /**
     * Flush the content stream, and close the reply. All data remaining
     * in the stream buffer is sent, and the stage move to CLOSE.
     * Note: Only valid at stage CONTENT.
     * @return true on success, else false
     */
    bool CloseContent();

    /**
     * Abort the reply, without flushing content or posting header.
     * Note that already posted data cannot be cancelled; and the reply
     * won't be reusable.
     * @return the stage before aborting
     */
    STAGE Abort();

  private:
    WSRequestBroker& m_broker;
    std::map<std::string, std::string> m_headers;
    STAGE m_stage;
    WSReplyChunked* m_chunked;

    // prevent copy
    WSRequestReply(const WSRequestReply&);
    WSRequestReply& operator=(const WSRequestReply&);
  };

}

#endif /* WSREQUESTREPLY_H */

