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

#include "flacencoder.h"
#include "private/byteorder.h"
#include "private/debug.h"

#define SAMPLES 1024

using namespace NSROOT;

FLACEncoder::FLACEncoder()
: AudioEncoder()
, m_open(false)
, m_ok(false)
, m_interleave(0)
, m_sampleSize(0)
, m_pcm(nullptr)
, m_encoder(nullptr)
, m_output(nullptr)
{
  m_encoder = new FLACEncoderPrivate(this);
}

FLACEncoder::~FLACEncoder()
{
  if (m_open)
    close();
  if (m_pcm != nullptr)
    delete[] m_pcm;
  delete m_encoder;
}

bool FLACEncoder::open(const AudioFormat& inputFormat, OutputStream * out)
{
  if (m_open)
  {
    DBG(DBG_WARN, "FLAC Encoder already opened\n");
    return false;
  }

  m_inputFormat = inputFormat;
  m_output = out;
  DBG(DBG_INFO, "Open FLAC encoder\n");

  // configure the encoder
  if (!(m_ok = m_inputFormat.isValid()))
    DBG(DBG_WARN, "ERROR: Invalid format\n");
  else if (!(m_ok = m_encoder->set_verify(true)))
    DBG(DBG_WARN, "ERROR: Set verify failed\n");
  else if (!(m_ok = m_encoder->set_compression_level(5)))
    DBG(DBG_WARN, "ERROR: Set compression level failed\n");
  else if (!(m_ok = m_encoder->set_channels(m_inputFormat.channelCount)))
    DBG(DBG_WARN, "ERROR: Set channels (%d) failed\n", m_inputFormat.channelCount);
  else if (!(m_ok = m_encoder->set_bits_per_sample(m_inputFormat.sampleSize)))
    DBG(DBG_WARN, "ERROR: Set sample size (%d) failed\n", m_inputFormat.sampleSize);
  else if (!(m_ok = m_encoder->set_sample_rate(m_inputFormat.sampleRate)))
    DBG(DBG_WARN, "ERROR: Set sample rate (%d) failed\n", m_inputFormat.sampleRate);
  else if (!(m_ok = (m_inputFormat.sampleSize == 8 && m_inputFormat.sampleType == AudioFormat::UnSignedInt) ||
          (m_inputFormat.sampleSize == 16 && m_inputFormat.sampleType == AudioFormat::SignedInt && m_inputFormat.byteOrder == AudioFormat::LittleEndian) ||
          (m_inputFormat.sampleSize == 24 && m_inputFormat.sampleType == AudioFormat::SignedInt && m_inputFormat.byteOrder == AudioFormat::LittleEndian) ||
          (m_inputFormat.sampleSize == 32 && m_inputFormat.sampleType == AudioFormat::SignedInt && m_inputFormat.byteOrder == AudioFormat::LittleEndian)
          ))
    DBG(DBG_WARN, "ERROR: Audio format not supported: %d%s%s\n", m_inputFormat.sampleSize,
            m_inputFormat.sampleType == AudioFormat::SignedInt ? "S" : "U",
            m_inputFormat.sampleSize > 8 ? m_inputFormat.byteOrder == AudioFormat::LittleEndian ? "LE" : "BE" : "");

  // the encoder only supports 24 bits, so the lower LSB will be removed
  if (m_inputFormat.sampleSize == 32)
    m_encoder->set_bits_per_sample(24);

  if (!m_ok)
    return false;

  m_interleave = m_inputFormat.bytesPerFrame() / m_inputFormat.channelCount;
  m_sampleSize = m_inputFormat.sampleSize;

  if (m_pcm != nullptr)
    delete[] m_pcm;
  m_pcm = new FLAC__int32 [SAMPLES * m_inputFormat.channelCount];

  m_open = true;
  FLAC__StreamEncoderInitStatus init_status = m_encoder->init();
  if(init_status == FLAC__STREAM_ENCODER_INIT_STATUS_OK)
    return true;
  close();
  DBG(DBG_WARN, "ERROR: initializing FLAC encoder: %s\n", FLAC__StreamEncoderInitStatusString[init_status]);
  return false;
}

void FLACEncoder::close()
{
  if (m_open)
  {
    DBG(DBG_INFO, "Close FLAC encoder\n");
    m_encoder->finish();
    m_open = false;
  }
}

int FLACEncoder::write(const char * data, int len)
{
  if (!m_open)
    return 0;

  bool ok = true;
  int samples = len / m_interleave / m_inputFormat.channelCount;
  while (ok && samples > 0)
  {
    int need = (samples > SAMPLES ? SAMPLES : static_cast<int>(samples));
    // convert the packed little-endian PCM samples into an interleaved FLAC__int32 buffer for libFLAC
    for(int i = 0; i < (need * m_inputFormat.channelCount); i++)
    {
      switch (m_sampleSize)
      {
      case 8:
        m_pcm[i] = (unsigned char)(*data) - 128;
        break;
      case 16:
        m_pcm[i] = read_b16le(data);
        break;
      case 24:
        m_pcm[i] = read_b24le(data);
        break;
      case 32:
        // remove lower LSB
        m_pcm[i] = (read_b32le(data) >> 8);
        break;
      default:
        m_pcm[i] = 0;
      }
      data += m_interleave;
    }
    // feed samples to encoder
    ok = m_encoder->process_interleaved(m_pcm, need);
    samples -= need;
  }
  return len;
}

int FLACEncoder::writeEncoded(const char * data, int len)
{
  if (m_output)
    return m_output->write(data, len);
  return 0;
}

FLAC__StreamEncoderWriteStatus FLACEncoder::FLACEncoderPrivate::write_callback(const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame)
{
  DBG(DBG_DEBUG, "FLAC encoder wrote %" PRIu64 " bytes, %u samples, %u frame\n", (uint64_t)bytes, samples, current_frame);
  int r = m_p->writeEncoded((const char*)buffer, (int)bytes);
  return (r == (int)bytes ? FLAC__STREAM_ENCODER_WRITE_STATUS_OK : FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR);
}
