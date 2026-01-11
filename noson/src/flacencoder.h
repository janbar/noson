/*
 *      Copyright (C) 2018-2026 Jean-Luc Barriere
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef FLACENCODER_H
#define FLACENCODER_H

#include "local_config.h"
#include "audioencoder.h"

#include <FLAC++/metadata.h>
#include <FLAC++/encoder.h>

namespace NSROOT
{

class FLACEncoder : public AudioEncoder
{
  friend class FLACEncoderPrivate;

public:
  FLACEncoder();
  ~FLACEncoder() override;

  std::string mediaType() const override { return "audio/x-flac"; }
  bool open(const AudioFormat& inputFormat, OutputStream * out) override;
  void close() override;

  int Write(const char* data, int len) override;

private:
  int writeEncoded(const char * data, int len);

  bool m_open;
  bool m_ok;
  int m_interleave;
  int m_sampleSize;
  FLAC__int32 * m_pcm;

  class FLACEncoderPrivate : public FLAC::Encoder::Stream
  {
  public:
    explicit FLACEncoderPrivate(FLACEncoder * p) : m_p(p) { }
    virtual FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame) override;
  private:
    FLACEncoder * m_p;
  };

  FLACEncoderPrivate * m_encoder;
  OutputStream * m_output;
  AudioFormat m_inputFormat;
};

}

#endif // FLACENCODER_H
