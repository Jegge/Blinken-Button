/*
 * animations.h
 *
 *  http://interactive-matter.org/
 *
 *  This file is part of Blinken Button.
 *
 *  Blinken Button is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Blinken Button is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  Created on: 26.01.2010
 */

#ifndef ANIMATIONS_H_
#define ANIMATIONS_H_

#include <avr/pgmspace.h>

//how many chars has the longest message
#define MAX_MESSAGE_LENGTH 40
//how often should we display messages
#define MESSAGE_PROBABILITY 10
//how much messages do we have
#define MAX_MESSAGE 3
//and now to the messages
const prog_char PROGMEM message_00[] PROGMEM = "INTERACTIVE-MATTER.ORG";
const prog_char PROGMEM message_01[] PROGMEM = "INSERT COIN";
const prog_char PROGMEM message_02[] PROGMEM = "SPACE INVADERS AGAINST RACISM";
const prog_char PROGMEM message_03[] PROGMEM = "";
PGM_P PROGMEM messages[] =
  { message_00, message_01, message_02 };
//a buffer for loading messages from flash
char message[MAX_MESSAGE_LENGTH];

//our animations
//first number is # sprites
const uint8_t sprite_0[] PROGMEM =
  { 2, 0, 1 };
const uint8_t sprite_1[] PROGMEM =
  { 2, 2, 3 };
const uint8_t sprite_2[] PROGMEM =
  { 2, 4, 5 };
const uint8_t sprite_3[] PROGMEM = 
  { 4, 6, 7, 8, 7 };
//a sequence is a animation + display speed an length
typedef struct
{
  uint8_t display_speed;
  uint8_t display_length;
  PGM_P sprites;
} _sequence_struct;

#define MAX_SEQUENCE 4
const _sequence_struct sequences[] PROGMEM =
  {
  //the first two sprites are defined twice - to ensure that they are shown more often
        { 7, 20, sprite_0 },
        { 7, 20, sprite_1 },
        { 7, 20, sprite_2 },
        { 7, 20, sprite_3 } 
   };

#endif /* ANIMATIONS_H_ */
