/*
 * display.c
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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "state.h"
#include "../flash-content/sprites.h"
#include "display.h"

//use dot correction?
//#define DOT_CORRECTION

#ifdef PNP_TRANSISTOR
#define PORTB_DRAIN_MASK _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4)
#define PORTC_DRAIN_MASK _BV(1) | _BV(2) | _BV(3) | _BV(4) | _BV(5)
#define PORTD_DRAIN_MASK _BV(2) | _BV(3) | _BV(4) | _BV(5) | _BV(6) | _BV(7)
#else
#define PORTB_DRAIN_MASK _BV(1) | _BV(3) | _BV(4)
#define PORTC_DRAIN_MASK _BV(1) | _BV(4)
#define PORTD_DRAIN_MASK _BV(2) | _BV(5) | _BV(7)
#endif

//load the default sequence in the internal buffer
//see RESERVED_SPRITES
void
display_load_default_sequence(void);
//internal functions
//start the main render timer
void
display_start_column_timer(void);
//internal routine to load sprite

//the current column, which is rendered
register uint8_t display_curr_column asm("r2");
uint8_t display_current_buffer;
//some small local display status
register uint8_t display_status asm("r3");
#define DISPLAY_BUFFER_LOCKED _BV(0)
#define DISPLAY_BUFFER_ADVANCE _BV(1)
#define DISPLAY_TEST_MODE _BV(2)
//to schedule advances for the test mode
uint8_t advance_counter;

//some our status for the prog led
#define PROG_LED_PIN _BV(5)
#define PROG_LED_PORT PORTB
#define PROG_LED_DDR DDRB
volatile uint8_t prog_led_status = 0;

typedef struct
{
  uint8_t pb;
  uint8_t pc;
  uint8_t pd;
  uint8_t num_bit;
} display_line;

//the sprite buffer for the calculated sprites
display_line display_buffer[2][8];

void
display_init()
{
  //set all unused pins as inputs & and all display pins as output
  DDRB = _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4);
  DDRC = _BV(1) | _BV(2) | _BV(3) | _BV(4) | _BV(5);
  DDRD = _BV(2) | _BV(3) | _BV(4) | _BV(5) | _BV(6) | _BV(7);

  //load the default sequence
  display_load_default_sequence();

  //but until we get data we go to testmode
  display_status |= DISPLAY_TEST_MODE;

  //kick off the display timers to start rendering
  display_start_column_timer();

  PROG_LED_DDR |= PROG_LED_PIN;
}

//load a sprite from the 8 bit buffer to the display struct
//the display struct contains all port settings to increase the render speed
void
display_load_sprite(uint8_t origin[])
{
  //we leave the testmode
  display_status &= ~(DISPLAY_TEST_MODE);
  //we select the next buffer by xoring either 0 or 1 with 1
  uint8_t number = display_current_buffer ^ 1;
  //lock the buffer to signal the display to wait a tad until we finished
  display_status |= DISPLAY_BUFFER_LOCKED;
  uint8_t column;
  for (column = 0; column < 8; column++)
    {
      //set all row drains to high
      uint8_t pb = PORTB_DRAIN_MASK;
      uint8_t pc = PORTC_DRAIN_MASK;
      uint8_t pd = PORTD_DRAIN_MASK;

      //select the correct column
      //this will switch on the column transistor
#ifdef PNP_TRANSISTOR
      if (column == 0)
        {
          pc &= ~_BV(5);
        }
      else if (column == 1)
        {
          pd &= ~_BV(6);
        }
      else if (column == 2)
        {
          pb &= ~_BV(0);
        }
      else if (column == 3)
        {
          pc &= ~_BV(2);
        }
      else if (column == 4)
        {
          pb &= ~_BV(2);
        }
      else if (column == 5)
        {
          pc &= ~_BV(3);
        }
      else if (column == 6)
        {
          pd &= ~_BV(3);
        }
      else if (column == 7)
        {
          pd &= ~_BV(4);
        };
#else
      if (column==0)
        {
          pc |= _BV(5);
        }
      else if (column==1)
        {
          pd |= _BV(6);
        }
      else if (column==2)
        {
          pb |= _BV(0);
        }
      else if (column==3)
        {
          pc |= _BV(2);
        }
      else if (column==4)
        {
          pb |= _BV(2);
        }
      else if (column==5)
        {
          pc |= _BV(3);
        }
      else if (column==6)
        {
          pd |= _BV(3);
        }
      else if (column==7)
        {
          pd |= _BV(4);
        };
#endif
      display_buffer[number][column].num_bit = 0;
      //enable the drain for the selected lines
      if (origin[0] & _BV(column))
        {
          pc &= ~_BV(1);
          display_buffer[number][column].num_bit++;
        }
      if (origin[1] & _BV(column))
        {
          pd &= ~_BV(2);
          display_buffer[number][column].num_bit++;
        }
      if (origin[2] & _BV(column))
        {
          pb &= ~_BV(4);
          display_buffer[number][column].num_bit++;
        }
      if (origin[3] & _BV(column))
        {
          pc &= ~_BV(4);
          display_buffer[number][column].num_bit++;
        }
      if (origin[4] & _BV(column))
        {
          pd &= ~_BV(5);
          display_buffer[number][column].num_bit++;
        }
      if (origin[5] & _BV(column))
        {
          pb &= ~_BV(3);
          display_buffer[number][column].num_bit++;
        }
      if (origin[6] & _BV(column))
        {
          pd &= ~_BV(7);
          display_buffer[number][column].num_bit++;
        }
      if (origin[7] & _BV(column))
        {
          pb &= ~_BV(1);
          display_buffer[number][column].num_bit++;
        }

      //save the calculated values to the sprite
      display_buffer[number][column].pb = pb;
      display_buffer[number][column].pc = pc;
      display_buffer[number][column].pd = pd;
    }
  //unlock the buffer
  display_status &= ~(DISPLAY_BUFFER_LOCKED);
}

//switch to the next buffer
void
display_advance_buffer(void)
{
  display_status |= DISPLAY_BUFFER_ADVANCE;
}
void
display_load_default_sequence()
{
  uint8_t default_load_buffer[8] =
    { 0, 0, 0, 0, 0, 0, 0, 0 };

  display_current_buffer = 0;
  copy_to_buffer(predefined_sprites[DEFAULT_1], default_load_buffer);
  display_load_sprite(default_load_buffer);
  display_current_buffer = 1;
  copy_to_buffer(predefined_sprites[DEFAULT_2], default_load_buffer);
  display_load_sprite(default_load_buffer);
  display_current_buffer = 0;
}

void display_prog_led_enable(void)
{
  prog_led_status = -1;
  PROG_LED_PORT |= PROG_LED_PIN;
}

void display_prog_led_disable(void)
{
  prog_led_status = 0;
  PROG_LED_PORT &= ~(PROG_LED_PIN);
}

void
display_start_column_timer(void)
{
  power_timer0_enable();
  //nothing to set on TCCR0A
  TCCR0A = 0;
  TCCR0B = _BV(CS02) || _BV(CS00);
  TIMSK0 = _BV(OCIE0A);
  OCR0A = 200;
}

ISR(TIMER0_COMPA_vect )
{
  //we want no other interrupts
  cli();
  //set all row drains to high
  PORTB = PORTB_DRAIN_MASK;
  PORTC = PORTC_DRAIN_MASK;
  PORTD = PORTD_DRAIN_MASK;
  //we strip the first two bits - they are for dimming
  //TODO do we get an 2 bit resolution too?
  uint8_t display_column = display_curr_column & 7;
#ifdef DOT_CORRECTION
  if ((display_curr_column & 8) && !(display_buffer[current_buffer][display_column].num_bit & 0xFC))
    {
      //do nothing - disable
      ;
    }
  else
#endif
    {
      //the set the ports line still off
      PORTB = display_buffer[display_current_buffer][display_column].pb
          | PORTB_DRAIN_MASK;
      PORTC = display_buffer[display_current_buffer][display_column].pc
          | PORTC_DRAIN_MASK;
      PORTD = display_buffer[display_current_buffer][display_column].pd
          | PORTD_DRAIN_MASK;
      //the set the ports line enable
      PORTB = display_buffer[display_current_buffer][display_column].pb;
      PORTC = display_buffer[display_current_buffer][display_column].pc;
      PORTD = display_buffer[display_current_buffer][display_column].pd;
    }

  //mask to max 8 + 2 dimming bits
  display_curr_column++;
  display_curr_column &= 0x1f;

  if (display_curr_column == 0)
    {
      //we must support the testmode
      if (display_status & DISPLAY_TEST_MODE)
        {
          advance_counter++;
          if (advance_counter == 0)
            {
              display_status |= DISPLAY_BUFFER_ADVANCE;
            }
        }

      //if we reached the last column (and wrap) & we should advance to the next sprite display the next sprite
      if ((display_status & (DISPLAY_BUFFER_LOCKED | DISPLAY_BUFFER_ADVANCE))
          == DISPLAY_BUFFER_ADVANCE)
        {
          //advance one buffer
          display_current_buffer = display_current_buffer ^ 1;
          display_status &= ~(DISPLAY_BUFFER_ADVANCE);
        }
    }

  if (prog_led_status)
    {
      PROG_LED_PORT |= PROG_LED_PIN;
    }
  else
    {
      PROG_LED_PORT &= ~(PROG_LED_PIN);
    }
  //now other interrupts are fine
  sei();
}

