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

/* the following two defines are not used (so, why are they here?) */
#define EPL_5700L_USB_EP_OUTBOUND  0x01
#define EPL_5700L_USB_EP_INBOUND   0x82
   
/* 
 Mythical replies from 5700l that needs to be dealt with 
 for USB printing on 5700L to work.
 5800L, 5900L, 6100L can print even if we don't read
 back the reply. Anyway, we do.

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
          case 0x11: result = 33; break; /* middle of polling */
          case 0x12: result = 19; break; /* polling */
          case 0x13: result = 20; break; /* 2nd command of job */
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
          case 0x11: result = 35; break; /* middle of polling */
          case 0x12: result = 19; break; /* polling */
          case 0x13: result = 20; break; /* 2nd command of job */
        }
    }
  else if (model == MODEL_6100L)
    {
      switch (code)
        {
	  /* big numbers for the time being */
	case 'B': result = 100; break; /* job header */
	case 'A': result = 100; break; /* job footer */
	case 'D': result = 100; break; /* 1st level encapsulation */
	case 'C': result = 100; break; /* 1st level decapsulation */
	case 'F': result = 100; break; /* page header */
	case 'E': result = 100; break; /* page footer */
	case 'I': result = 0;  break;  /* strip */
	case '@': result = 100; break; /* pre header */ 
        }
    }
  return result;
}