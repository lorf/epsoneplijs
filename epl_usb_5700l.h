/*
  Copyright (c) 2003 Hin-Tak Leung
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

/* 
 these two should be obtained from the device and stored 
 in epl_job_info, rather than hard-coded here.
*/ 

#define EPL_5700L_USB_EP_OUTBOUND  0x01
#define EPL_5700L_USB_EP_INBOUND   0x82

/* 
 Mythical replies from 5700l that needs to be dealt with 
 for USB printing on 5700L to work.

 The reply size seems to be simply related to the first
 byte of a 2-byte command, hence we just use the first
 byte to index the size. 
*/

static int epl_5700l_reply_size[] = 
  {
    15, /* 0x00, job header     */
    15, /* 0x01, job footer     */
    15, /* 0x02, page header    */
    15, /* 0x03, page footer    */
    0 , /* 0x04, strip, but strip doesn't require bid; not used  */ 
    21, /* 0x05, 2nd command of job  */
    19, /* 0x06, beginning   of job  */ 
    28, /* 0x07, middle of job       */
    17  /* 0x08, some kind of polling, sent when the printer is idle */   
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
