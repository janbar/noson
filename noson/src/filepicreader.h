/*
 *      Copyright (C) 2018-2019 Jean-Luc Barriere
 *
 *  This file is part of Noson
 *
 *  Noson is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Noson is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef FILEPICREADER_H
#define FILEPICREADER_H

#include "local_config.h"

#include "streamreader.h"

#include <cstdint>
#include <string>
#include <vector>

#define FILEPICREADER_PARAM_PATH  "path"
#define FILEPICREADER_PARAM_TYPE  "type"

namespace NSROOT
{

class FilePicReader : public StreamReader
{
private:
  struct Picture
  {
    void * payload;
    void (*free)(void * payload);
    const char * mime;
    const char * data;
    unsigned size;
    Picture();
    ~Picture();
  };
  static FilePicReader _instance;
  FilePicReader();
  ~FilePicReader() override { }

public:
  static FilePicReader * Instance();

  STREAM * OpenStream(const std::string& streamUrl) override;
  int ReadStream(STREAM * stream) override;
  void CloseStream(STREAM * stream) override;

  enum PictureType // according to the ID3v2 APIC frame
  {
    Any           =-1,
    Other         = 0,
    fileIcon      = 1,
    OtherFileIcon = 2,
    CoverFront    = 3,
    CoverBack     = 4,
    LeafletPage   = 5,
    Media         = 6,
    LeadArtist    = 7,
    Artist        = 8,
    Conductor     = 9,
    Orchestra     = 10,
    Composer      = 11,
    Lyricist      = 12,
  };

private:
  static void readParameters(const std::string& streamUrl, std::vector<std::string>& params);
  static std::string getParamValue(const std::vector<std::string>& params, const std::string& name);

  static Picture * ExtractFLACPicture(const std::string& filePath, PictureType pictureType, bool& error);
  static void FreeFLACPicture(void * payload);

  static Picture * ExtractID3Picture(const std::string& filePath, PictureType pictureType, bool& error);
  static void FreeID3Picture(void * payload);
  static int parse_id3v2(FILE * file, long id3v2_offset, Picture ** pic, off_t * ptag_size, PictureType pictureType);
  static long find_id3v2(FILE * file, off_t * sync_offset);
  static int parse_id3v2_pic_v2(FILE * file, unsigned frame_size, Picture ** pic, PictureType pictureType);
  static int parse_id3v2_pic_v3(FILE * file, unsigned frame_size, Picture ** pic, PictureType pictureType);

  static Picture * ExtractOGGSPicture(const std::string& filePath, PictureType pictureType, bool& error);
  static void FreeOGGSPicture(void * payload);
  typedef struct {
    unsigned char * buf;
    uint32_t size;
    unsigned char * data;
    uint32_t datalen;
  } packet_t;
  static bool resize_packet(packet_t * packet, uint32_t size);
  static bool fill_packet(packet_t * packet, uint32_t len, FILE * fp);
  static bool parse_comment(packet_t * packet, Picture ** pic, PictureType pictureType);

  static Picture * ExtractMP4Picture(const std::string& filePath, PictureType pictureType, bool& error);
  static void FreeMP4Picture(void * payload);
  static int nextChild(unsigned char * buf, uint64_t * remaining, FILE * fp, unsigned * child, uint64_t * childSize);
  static int loadDataValue(uint64_t * remaining, FILE * fp, char ** alloc, unsigned * allocSize);
  static int loadCovrValue(uint64_t * remaining, FILE * fp, Picture ** pic);
  static int parse_ilst(uint64_t * remaining, FILE * fp, Picture ** pic);
  static int parse_meta(uint64_t * remaining, FILE * fp, Picture ** pic);
  static int parse_udta(uint64_t * remaining, FILE * fp, Picture ** pic);
  static int parse_moov(uint64_t * remaining, FILE * fp, Picture ** pic);
};

}

#endif /* FILEPICREADER_H */

