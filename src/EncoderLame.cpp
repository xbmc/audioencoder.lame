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
#include "xbmc_audioenc_dll.h"
#include <string.h>
#include <stdlib.h>
#include <algorithm>

// global settings
int preset=-1;
int bitrate;

extern "C" {
//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  return ADDON_STATUS_NEED_SETTINGS;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return true;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  if (strcmp(strSetting,"preset") == 0)
  {
    int ival = *((int*)value);
    if (ival == 0)
      preset = MEDIUM;
    else if (ival == 1)
      preset = STANDARD;
    else if (ival == 2)
      preset = EXTREME;
  }
  if (strcmp(strSetting,"bitrate") == 0)
  {
    int ival = *((int*)value);
    bitrate = 128 + 32 * ival;
  }
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

// class to hold lame context
class lame_context
{
public:
  lame_context(audioenc_callbacks &cb, lame_global_flags *enc) :
    callbacks(cb),
    encoder(enc),
    audio_pos(0)
  {
  }

  audioenc_callbacks callbacks;     ///< callback structure for write/seek etc.
  lame_global_flags* encoder;       ///< lame encoder context
  int                audio_pos;     ///< audio position in file
  uint8_t            buffer[65536]; ///< buffer for writing out audio data
};


void* Create(audioenc_callbacks *callbacks)
{
  if (callbacks && callbacks->write)
  {
    lame_global_flags *enc = lame_init();
    if (!enc)
      return NULL;

    if (preset == -1)
      lame_set_brate(enc, bitrate);
    else
      lame_set_preset(enc, preset);

    lame_set_asm_optimizations(enc, MMX, 1);
    lame_set_asm_optimizations(enc, SSE, 1);

    return new lame_context(*callbacks, enc);
  }
  return NULL;
}

bool Start(void* ctx, int iInChannels, int iInRate, int iInBits,
          const char* title, const char* artist,
          const char* albumartist, const char* album,
          const char* year, const char* track, const char* genre,
          const char* comment, int tracklength)
{
  lame_context *context = (lame_context*)ctx;
  if (!context)
    return false;

  // we accept only 2 ch 16 bit atm
  if (iInChannels != 2 || iInBits != 16)
    return false;

  lame_set_in_samplerate(context->encoder, iInRate);

  // disable automatic ID3 tag writing - we'll write ourselves
  lame_set_write_id3tag_automatic(context->encoder, 0);

  // Setup the ID3 tagger
  id3tag_init(context->encoder);
  id3tag_add_v2(context->encoder);
  id3tag_set_title(context->encoder, title);
  id3tag_set_artist(context->encoder, artist);
  id3tag_set_textinfo_latin1(context->encoder, "TPE2", albumartist);
  id3tag_set_album(context->encoder, album);
  id3tag_set_year(context->encoder, year);
  id3tag_set_track(context->encoder, track);
  int test = id3tag_set_genre(context->encoder, genre);
  if(test==-1)
    id3tag_set_genre(context->encoder,"Other");

  // Now that all the options are set, lame needs to analyze them and
  // set some more internal options and check for problems
  if (lame_init_params(context->encoder) < 0)
  {
    return false;
  }

  // now write the ID3 tag information, storing the position
  int tag_length = lame_get_id3v2_tag(context->encoder, context->buffer, sizeof(context->buffer));
  if (tag_length)
  {
    context->callbacks.write(context->callbacks.opaque, context->buffer, tag_length);
    context->audio_pos = tag_length;
  }

  return true;
}

int Encode(void* ctx, int nNumBytesRead, uint8_t* pbtStream)
{
  lame_context *context = (lame_context*)ctx;
  if (!context)
    return -1;

  // note: assumes 2ch 16bit atm
  const int bytes_per_frame = 2*2;

  int bytes_left = nNumBytesRead;
  while (bytes_left)
  {
    const int frames = std::min(bytes_left / bytes_per_frame, 4096);

    int written = lame_encode_buffer_interleaved(context->encoder, (short*)pbtStream, frames, context->buffer, sizeof(context->buffer));
    if (written < 0)
      return -1; // error
    context->callbacks.write(context->callbacks.opaque, context->buffer, written);

    pbtStream  += frames * bytes_per_frame;
    bytes_left -= frames * bytes_per_frame;
  }

  return nNumBytesRead - bytes_left;
}

bool Finish(void* ctx)
{
  lame_context *context = (lame_context*)ctx;
  if (!context)
    return false;

  // may return one more mp3 frames
  int written = lame_encode_flush(context->encoder, context->buffer, sizeof(context->buffer));
  if (written < 0)
    return false;

  context->callbacks.write(context->callbacks.opaque, context->buffer, written);

  // write id3v1 tag to file
  int id3v1tag = lame_get_id3v1_tag(context->encoder, context->buffer, sizeof(context->buffer));
  if (id3v1tag > 0)
    context->callbacks.write(context->callbacks.opaque, context->buffer, id3v1tag);

  // update LAME/Xing tag
  int lameTag = lame_get_lametag_frame(context->encoder, context->buffer, sizeof(context->buffer));
  if (context->audio_pos && lameTag > 0)
  {
    context->callbacks.seek(context->callbacks.opaque, context->audio_pos, SEEK_SET);
    context->callbacks.write(context->callbacks.opaque, context->buffer, lameTag);
  }

  return true;
}

void Free(void *ctx)
{
  lame_context *context = (lame_context*)ctx;
  if (context)
    lame_close(context->encoder);
}
}
