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
#include "xbmc/xbmc_audioenc_dll.h"
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
  lame_context(audioenc_callbacks &cb, lame_global_flags *flgs) :
    callbacks(cb),
    flags(flgs)
  {
  }

  audioenc_callbacks callbacks;   ///< callback structure for write/seek etc.
  lame_global_flags* flags;       ///< lame encoder global flags
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

  lame_set_in_samplerate(context->flags, iInRate);

  // Setup the ID3 tagger
  id3tag_init(context->flags);
  id3tag_add_v2(context->flags);
  id3tag_set_title(context->flags, title);
  id3tag_set_artist(context->flags, artist);
  id3tag_set_textinfo_latin1(context->flags, "TPE2", albumartist);
  id3tag_set_album(context->flags, album);
  id3tag_set_year(context->flags, year);
  id3tag_set_track(context->flags, track);
  int test = id3tag_set_genre(context->flags, genre);
  if(test==-1)
    id3tag_set_genre(context->flags,"Other");

  // Now that all the options are set, lame needs to analyze them and
  // set some more internal options and check for problems
  if (lame_init_params(context->flags) < 0)
  {
    return false;
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

  // buffer for encoded audio. Should be at least 1.25*num_samples + 7200
  uint8_t  buffer[65536];

  int bytes_left = nNumBytesRead;
  while (bytes_left)
  {
    const int frames = std::min(bytes_left / bytes_per_frame, 4096);

    int written = lame_encode_buffer_interleaved(context->flags, (short*)pbtStream, frames, buffer, sizeof(buffer));
    if (written < 0)
      return -1; // error
    context->callbacks.write(context->callbacks.opaque, buffer, written);

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

  // buffer for encoded audio.
  uint8_t  buffer[65536];

  // may return one more mp3 frames
  int written = lame_encode_flush(context->flags, buffer, sizeof(buffer));
  if (written < 0)
    return false;

  context->callbacks.write(context->callbacks.opaque, buffer, written);

  // TODO: add VBR tags to mp3 file...
/*
  FILE* file = fopen(File, "rb+");
  if (!file)
  {
    return false;
  }

  lame_mp3_tags_fid(context->flags, file);
  fclose(file);
*/

  return true;
}

void Free(void *ctx)
{
  lame_context *context = (lame_context*)ctx;
  if (context)
    lame_close(context->flags);
}
}
