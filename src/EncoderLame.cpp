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

lame_global_flags* m_pGlobalFlags;
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

bool Init(int iInChannels, int iInRate, int iInBits,
          const char* title, const char* artist,
          const char* albumartist, const char* album,
          const char* year, const char* track, const char* genre,
          const char* comment, int tracklength)
{
  // we only accept 2 / 44100 / 16 atm
  if (iInChannels != 2 || iInRate != 44100 || iInBits != 16) return false;

  m_pGlobalFlags = lame_init();
  if (!m_pGlobalFlags)
  {
    return false;
  }

  if (preset == -1)
    lame_set_brate(m_pGlobalFlags, bitrate);
  else
    lame_set_preset(m_pGlobalFlags, preset);


  lame_set_asm_optimizations(m_pGlobalFlags, MMX, 1);
  lame_set_asm_optimizations(m_pGlobalFlags, SSE, 1);
  lame_set_in_samplerate(m_pGlobalFlags, 44100);

  // Setup the ID3 tagger
  id3tag_init(m_pGlobalFlags);
  id3tag_add_v2(m_pGlobalFlags);
  id3tag_set_title(m_pGlobalFlags, title);
  id3tag_set_artist(m_pGlobalFlags, artist);
  id3tag_set_textinfo_latin1(m_pGlobalFlags, "TPE2", albumartist);
  id3tag_set_album(m_pGlobalFlags, album);
  id3tag_set_year(m_pGlobalFlags, year);
  id3tag_set_track(m_pGlobalFlags, track);
  int test = id3tag_set_genre(m_pGlobalFlags, genre);
  if(test==-1)
    id3tag_set_genre(m_pGlobalFlags,"Other");
  // Now that all the options are set, lame needs to analyze them and
  // set some more internal options and check for problems
  if (lame_init_params(m_pGlobalFlags) < 0)
  {
    return false;
  }

  return true;
}

int Encode(int nNumBytesRead, uint8_t* pbtStream, uint8_t* buffer)
{
  return lame_encode_buffer_interleaved(m_pGlobalFlags, (short*)pbtStream, nNumBytesRead / 4, buffer, 128*1024*1024);
}

int Flush(uint8_t* buffer)
{
  // may return one more mp3 frames
  return lame_encode_flush(m_pGlobalFlags, buffer, 128*1024*1024);
}

bool Close(const char* File)
{
  // open again, but now the old way, lame only accepts FILE pointers
  FILE* file = fopen(File, "rb+");
  if (!file)
  {
    return false;
  }

  lame_mp3_tags_fid(m_pGlobalFlags, file); /* add VBR tags to mp3 file */
  fclose(file);

  lame_close(m_pGlobalFlags);

  return true;
}
}
