/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <signal.h>

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/router.h"
#include "audio/midi.h"
#include "audio/midi_event.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/objects.h"

/**
 * Saves a string representation of the given
 * control change event in the given buffer.
 *
 * @param buf The string buffer to fill, or NULL
 *   to only get the channel.
 *
 * @return The MIDI channel, or -1 if not ctrl
 *   change.
 */
int
midi_ctrl_change_get_ch_and_description (
  midi_byte_t * ctrl_change,
  char *        buf)
{
  /* assert the given event is a ctrl change event */
  if (ctrl_change[0] < 0xB0 ||
      ctrl_change[0] > 0xBF)
    {
      return -1;
    }

  if (buf)
    {
      if (ctrl_change[1] >= 0x08 &&
          ctrl_change[1] <= 0x1F)
        {
          sprintf (
            buf, "Continuous controller #%u",
            ctrl_change[1]);
        }
      else if (ctrl_change[1] >= 0x28 &&
               ctrl_change[1] <= 0x3F)
        {
          sprintf (
            buf, "Continuous controller #%u",
            ctrl_change[1] - 0x28);
        }
      else
        {
          switch (ctrl_change[1])
            {
            case 0x00:
            case 0x20:
              strcpy (
                buf, "Continuous controller #0");
              break;
            case 0x01:
            case 0x21:
              strcpy (buf, "Modulation wheel");
              break;
            case 0x02:
            case 0x22:
              strcpy (buf, "Breath control");
              break;
            case 0x03:
            case 0x23:
              strcpy (
                buf, "Continuous controller #3");
              break;
            case 0x04:
            case 0x24:
              strcpy (buf, "Foot controller");
              break;
            case 0x05:
            case 0x25:
              strcpy (buf, "Portamento time");
              break;
            case 0x06:
            case 0x26:
              strcpy (buf, "Data Entry");
              break;
            case 0x07:
            case 0x27:
              strcpy (buf, "Main Volume");
              break;
            default:
              strcpy (buf, "Unknown");
              break;
            }
        }
    }
  return (ctrl_change[0] - 0xB0) + 1;
}

/**
 * Used for MIDI controls whose values are split
 * between MSB/LSB.
 *
 * @param lsb First byte (pos 1).
 * @param msb Second byte (pos 2).
 */
int
midi_combine_bytes_to_int (
  midi_byte_t lsb,
  midi_byte_t msb)
{
  /* http://midi.teragonaudio.com/tech/midispec/wheel.htm */
  return (int) ((msb << 7) | lsb);
}

/**
 * Used for MIDI controls whose values are split
 * between MSB/LSB.
 *
 * @param lsb First byte (pos 1).
 * @param msb Second byte (pos 2).
 */
void
midi_get_bytes_from_int (
  int           val,
  midi_byte_t * lsb,
  midi_byte_t * msb)
{
  /* https://arduino.stackexchange.com/questions/18955/how-to-send-a-pitch-bend-midi-message-using-arcore */
  *lsb = val & 0x7F;
  *msb = val >> 7;
}

/**
 * Queues MIDI note off to event queues.
 *
 * @param queued Send the event to queues instead
 *   of main events.
 */
void
midi_panic_all (
  int queued)
{
  midi_events_panic (
    AUDIO_ENGINE->midi_editor_manual_press->
      midi_events, queued);

  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (track_has_piano_roll (track))
        {
          midi_events_panic (
            track->processor->piano_roll->
              midi_events,
            queued);
        }
    }
}

/**
 * Returns the length of the MIDI message based on
 * the status byte.
 *
 * TODO move this and other functions to utils/midi,
 * split separate files for MidiEvents and MidiEvent.
 */
int
midi_get_msg_length (
  uint8_t status_byte)
{
  // define these with meaningful names
  switch (status_byte & 0xf0) {
  case 0x80:
  case 0x90:
  case 0xa0:
  case 0xb0:
  case 0xe0:
    return 3;
  case 0xc0:
  case 0xd0:
    return 2;
  case 0xf0:
    switch (status_byte) {
    case 0xf0:
      return 0;
    case 0xf1:
    case 0xf3:
      return 2;
    case 0xf2:
      return 3;
    case 0xf4:
    case 0xf5:
    case 0xf7:
    case 0xfd:
      break;
    default:
      return 1;
    }
  }
  g_return_val_if_reached (-1);
}
