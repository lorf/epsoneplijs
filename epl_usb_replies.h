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

/* the following two defines are not used (so, why are they here?) */
#define EPL_5700L_USB_EP_OUTBOUND  0x01
#define EPL_5700L_USB_EP_INBOUND   0x82
   
/* 
 Mythical replies from 5700l that needs to be dealt with 
 for USB printing on 5700L to work.
 5800L, 5900L, 6100L can print even if we don't read
 back the reply. Anyway, we do.

 The reply size seems to be simply related to the first
 byte of a 2-byte command, hence we just use the first
 byte to index the size. 

 -1 should never occur.
 0 just means no reply is expected.
*/

#define KNOWN_REPLY_CODES 0x14

static int epl_usb_reply_size[MODEL_USEDSLOTS][KNOWN_REPLY_CODES] = 
  {
    {
      /* unknown printer */
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 /* 0x00 - 0x13, not used */
    },
    {
      /* 5700L */
      15, /* 0x00, job header */
      15, /* 0x01, job footer */
      15, /* 0x02, page header */
      15, /* 0x03, page footer */
      0,  /* 0x04, strip, but strip doesn't require bid; not used */
      21, /* 0x05, 2nd command of job */
      19, /* 0x06, beginning of job */ 
      28, /* 0x07, middle of job */
      17, /* 0x08, some kind of polling, sent when the printer is idle */   
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 /* 0x09 - 0x13, not used */
    },
    {
      /* 5800L */
      18, /* 0x00, job header */
      18, /* 0x01, job footer */
      18, /* 0x02, 1st level encapsulation */
      18, /* 0x03, 1st level decapsulation */
      18, /* 0x04, page header */
      18, /* 0x05, page footer */
      0,  /* 0x06, strip */
      -1, /* 0x07, not used */
      -1, -1, -1, -1, -1, -1, -1, -1, /* 0x08 - 0x0f, not used */
      33, /* 0x10, beginning of job */
      33, /* 0x11, middle of polling */ 
      19, /* 0x12, polling */
      20  /* 0x13, 2nd command of job */   
    },
    {
      /* 5900L */
      18, /* 0x00, job header */
      18, /* 0x01, job footer */
      18, /* 0x02, 1st level encapsulation */
      18, /* 0x03, 1st level decapsulation */
      18, /* 0x04, page header */
      18, /* 0x05, page footer */
      0,  /* 0x06, strip */
      -1,  /* 0x07, not used */
      -1, -1, -1, -1, -1, -1, -1, -1, /* 0x08 - 0x0f, not used */
      33, /* 0x10, beginning of job */
      33, /* 0x11, middle of polling */ 
      19, /* 0x12, polling */
      20  /* 0x13, 2nd command of job */   
    },
    {
      /* empty slot */
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 /* 0x00 - 0x13, not used */
    },
    {
      /* 6100L */
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 /* 0x00 - 0x13, unknown */
    }
  };

/* 
   Flow control parameter:
   According to the spec, the 5700L can do 8 pages per minutes; i.e. 
   7 seconds per page; the max page length is about 14in, containing about 
   140 stripes at max resolution. So the printer must be able to process
   a stripe within 0.05 seconds, or 50,000 micro-seconds. Hence this 
   number. One may need to increase or look for problems elsewhere
   if a value of 50,000 doesn't work reliably.

   The smallest value chosen so there is no unnecessary waits.
   The wait route will use this value for high resolution, and
   double it when the vertical resolution is only 300dpi. 
   (only 300 or 600 are valid values)  
*/
   
#define USEC_BETWEEN_WRITES_5700L 50000
