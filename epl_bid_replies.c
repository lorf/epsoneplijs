/*
  Copyright (c) 2003 Hin-Tak Leung, Roberto Ragusa
*/  

/**
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

#include "epl_job.h"
#include "epl_bid.h"

   
/* 
   Mythical replies from 5700l that needs to be dealt with 
   for USB printing on 5700L to work.
   5800L, 5900L, 6100L, 6200L can print even if we don't read
   back the reply. Anyway, we do because there are some
   advantages (smart flow control and status feedback are
   possible).

   The reply size seems to be simply related to the first
   byte of a 2-byte command. The caller passes the value
   through "code".

   -1 should never occur.
   0 just means no reply is expected.
*/

int epl_bid_reply_len(int model, unsigned char code)
{
  int result;
  
  result = -1; /* unknown, this can be returned if not overwritten */
  if (model == MODEL_5700L)
    {
      switch (code)
        {
        case 0x00: result = 15; break; /* job header */
        case 0x01: result = 15; break; /* job footer */
        case 0x02: result = 15; break; /* page header */
        case 0x03: result = 15; break; /* page footer */
        case 0x04: result = 0;  break; /* strip, but strip doesn't require bid */
        case 0x05: result = 21; break; /* 2nd command of job */
        case 0x06: result = 19; break; /* beginning of job */
        case 0x07: result = 28; break; /* middle of job */
        case 0x08: result = 17; break; /* some kind of polling, sent when the printer is idle */
        case 0x0d: result = 15; break; /* set time to standby */
          /*  To set standby time, send 0x0d 0x00, then "SET STANDBY" followed
              by number (default 3) of 10 minutes. Zero means disabling 
              standby. Other such commands are:  
              RESET OPCC
              RESET TNRC
              INIT EEPROM
          */
        }
    }
  else if (model == MODEL_5800L)
    {
      switch (code)
        {
        case 0x00: result = 18; break; /* job header */
        case 0x01: result = 18; break; /* job footer */
        case 0x02: result = 18; break; /* 1st level encapsulation */
        case 0x03: result = 18; break; /* 1st level decapsulation */
        case 0x04: result = 18; break; /* page header */
        case 0x05: result = 18; break; /* page footer */
        case 0x06: result = 0;  break; /* strip */
        case 0x10: result = 33; break; /* beginning of job */
        case 0x11: result = 33; break; /* middle of polling - different between 58/59 */
        case 0x12: result = 19; break; /* polling */
        case 0x13: result = 20; break; /* 2nd command of job */
        case 0x1b: result = 0;  break; /* EJL initialization or closing */
        case 0x40: result = 0;  break; /* EJL initialization or closing */
        }
    }
  else if (model == MODEL_5900L)
    {
      switch (code)
        {
        case 0x00: result = 18; break; /* job header */
        case 0x01: result = 18; break; /* job footer */
        case 0x02: result = 18; break; /* 1st level encapsulation */
        case 0x03: result = 18; break; /* 1st level decapsulation */
        case 0x04: result = 18; break; /* page header */
        case 0x05: result = 18; break; /* page footer */
        case 0x06: result = 0;  break; /* strip */
        case 0x10: result = 33; break; /* beginning of job */
        case 0x11: result = 35; break; /* middle of polling - - different between 58/59 */
        case 0x12: result = 19; break; /* polling */
        case 0x13: result = 20; break; /* 2nd command of job */
        case 0x1b: result = 0;  break; /* EJL initialization or closing */
        case 0x40: result = 0;  break; /* EJL initialization or closing */
        }
    }
  else if (model == MODEL_6100L)
    {
      switch (code)
        {
          /* big numbers for the time being */
        case 0x40: result = 16;  break; /* '@' - job header */ 
        case 0x41: result = 16;  break; /* 'A' - job footer */
        case 0x42: result = 16;  break; /* 'B' - 1st level encapsulation */
        case 0x43: result = 16;  break; /* 'C' - 1st level decapsulation */
        case 0x46: result = 16;  break; /* 'F' - page header */
        case 0x47: result = 16;  break; /* 'G' - page footer */
        case 0x4C: result = 0;   break; /* 'L' - strip */
        case 0x50: result = 126; break; /* 'P' - poll? */
        case 0x51: result = 120; break; /* 'Q' - poll? */
        case 0x52: result = 18;  break; /* 'R' - poll? */
        case 0x7f: result = 16;  break; /*     - ? */
        case 0x1b: result = 0;   break; /* EJL initialization or closing */
        }
    }
  else if (model == MODEL_6200L)
    {
      switch (code)
        {
          /* HTL: since July 2004 france trip's first-hand experience
             with a 6200L, 6200L data should now be regarded as more
             authoritative, where 6100L had been more guess work. 
          */
        case 0x40: result = 16;  break; /* '@' - job header ; 66-byte arg*/ 
        case 0x41: result = 16;  break; /* 'A' - job footer */
        case 0x42: result = 16;  break; /* 'B' - 1st level encapsulation ; 14-byte arg */
        case 0x43: result = 16;  break; /* 'C' - 1st level decapsulation ; */
          /* no D and E */
        case 0x46: result = 16;  break; /* 'F' - page header; 34-byte args */
        case 0x47: result = 16;  break; /* 'G' - page footer */
          /* */
        case 0x4C: result = 0;   break; /* 'L' - stripe */
          /* */
        case 0x4F: result = 16;  break; /* 'O' - poll - between @ and B ; takes 8-byte argument */
        case 0x50: result = 126; break; /* 'P' - poll - PQP every 10 sec while idle */
        case 0x51: result = 120; break; /* 'Q' - poll */
        case 0x52: result = 18;  break; /* 'R' - poll - before job header @ */
        case 0x60: result = 16;  break; /* backtick - sleepmode on,off, toner low cont on,off; 6-byte arg*/
        case 0x63: result = 16;  break; /* 'c' reset opc level ; 2-byte arg */
        case 0x71: result = 16;  break;  /* 'q' */
        case 0x7f: result = 16;  break; /* DEL - poll - between B/F and F/L - check connectivity? every sec */
        case 0x1b: result = 0;   break; /* EJL initialization or closing */
        }
    }
  return result;
}
