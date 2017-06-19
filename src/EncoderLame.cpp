/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <lame/lame.h>
#include <kodi/addon-instance/AudioEncoder.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>

class CEncoderLame : public kodi::addon::CInstanceAudioEncoder
{
public:
  CEncoderLame(KODI_HANDLE instance);
  virtual ~CEncoderLame();

  virtual bool Start(int inChannels,
                     int inRate,
                     int inBits,
                     const std::string& title,
                     const std::string& artist,
                     const std::string& albumartist,
                     const std::string& album,
                     const std::string& year,
                     const std::string& track,
                     const std::string& genre,
                     const std::string& comment,
                     int trackLength) override;
  virtual int Encode(int numBytesRead, const uint8_t* stream) override;
  virtual bool Finish() override;

private:
  lame_global_flags* m_encoder;       ///< lame encoder context
  int                m_audio_pos;     ///< audio position in file
  uint8_t            m_buffer[65536]; ///< buffer for writing out audio data
  int                m_preset;
  int                m_bitrate;
};


CEncoderLame::CEncoderLame(KODI_HANDLE instance)
  : CInstanceAudioEncoder(instance),
    m_audio_pos(0),
    m_preset(-1)
{
  m_encoder = lame_init();
  if (!m_encoder)
  {
    kodi::Log(ADDON_LOG_ERROR, "Failed to construct lame stream encoder");
    return;
  }

  int value = kodi::GetSettingInt("preset");
  if (value == 0)
    m_preset = MEDIUM;
  else if (value == 1)
    m_preset = STANDARD;
  else if (value == 2)
    m_preset = EXTREME;

  m_bitrate = 128 + 32 * kodi::GetSettingInt("bitrate");

  if (m_preset == -1)
    lame_set_brate(m_encoder, m_bitrate);
  else
    lame_set_preset(m_encoder, m_preset);

  lame_set_asm_optimizations(m_encoder, MMX, 1);
  lame_set_asm_optimizations(m_encoder, SSE, 1);
}

CEncoderLame::~CEncoderLame()
{
  lame_close(m_encoder);
}

bool CEncoderLame::Start(int inChannels, int inRate, int inBits,
                         const std::string& title, const std::string& artist,
                         const std::string& albumartist, const std::string& album,
                         const std::string& year, const std::string& track, const std::string& genre,
                         const std::string& comment, int trackLength)
{
  if (!m_encoder)
    return false;

  // we accept only 2 ch 16 bit atm
  if (inChannels != 2 || inBits != 16)
  {
    kodi::Log(ADDON_LOG_ERROR, "Invalid input format to encode");
    return false;
  }

  lame_set_in_samplerate(m_encoder, inRate);

  // disable automatic ID3 tag writing - we'll write ourselves
  lame_set_write_id3tag_automatic(m_encoder, 0);

  // Setup the ID3 tagger
  id3tag_init(m_encoder);
  id3tag_add_v2(m_encoder);
  id3tag_set_title(m_encoder, title.c_str());
  id3tag_set_artist(m_encoder, artist.c_str());
  id3tag_set_textinfo_latin1(m_encoder, "TPE2", albumartist.c_str());
  id3tag_set_album(m_encoder, album.c_str());
  id3tag_set_year(m_encoder, year.c_str());
  id3tag_set_track(m_encoder, track.c_str());
  int test = id3tag_set_genre(m_encoder, genre.c_str());
  if(test==-1)
    id3tag_set_genre(m_encoder, "Other");

  // Now that all the options are set, lame needs to analyze them and
  // set some more internal options and check for problems
  if (lame_init_params(m_encoder) < 0)
  {
    return false;
  }

  // now write the ID3 tag information, storing the position
  int tag_length = lame_get_id3v2_tag(m_encoder, m_buffer, sizeof(m_buffer));
  if (tag_length)
  {
    Write(m_buffer, tag_length);
    m_audio_pos = tag_length;
  }

  return true;
}

int CEncoderLame::Encode(int numBytesRead, const uint8_t* stream)
{
  if (!m_encoder)
    return -1;

  // note: assumes 2ch 16bit atm
  const int bytes_per_frame = 2*2;

  int bytes_left = numBytesRead;
  while (bytes_left)
  {
    const int frames = std::min(bytes_left / bytes_per_frame, 4096);

    int written = lame_encode_buffer_interleaved(m_encoder, (short*)stream, frames, m_buffer, sizeof(m_buffer));
    if (written < 0)
      return -1; // error
    Write(m_buffer, written);

    stream += frames * bytes_per_frame;
    bytes_left -= frames * bytes_per_frame;
  }

  return numBytesRead - bytes_left;
}

bool CEncoderLame::Finish()
{
  if (!m_encoder)
    return false;

  // may return one more mp3 frames
  int written = lame_encode_flush(m_encoder, m_buffer, sizeof(m_buffer));
  if (written < 0)
    return false;

  Write(m_buffer, written);

  // write id3v1 tag to file
  int id3v1tag = lame_get_id3v1_tag(m_encoder, m_buffer, sizeof(m_buffer));
  if (id3v1tag > 0)
    Write(m_buffer, id3v1tag);

  // update LAME/Xing tag
  int lameTag = lame_get_lametag_frame(m_encoder, m_buffer, sizeof(m_buffer));
  if (m_audio_pos && lameTag > 0)
  {
    Seek(m_audio_pos, SEEK_SET);
    Write(m_buffer, lameTag);
  }

  return true;
}

//------------------------------------------------------------------------------

class CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() { }
  virtual ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance) override;
};

ADDON_STATUS CMyAddon::CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance)
{
  addonInstance = new CEncoderLame(instance);
  return ADDON_STATUS_OK;
}

ADDONCREATOR(CMyAddon)
