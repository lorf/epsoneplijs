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
  The content of this file is just kept for reference;
  Not used anywhere, as the 5800l and 5900l seems
  to work uni-directionally, unlike the 5700l. 
*/

/* 
 these two should be obtained from the device and stored 
 in epl_job_info, rather than hard-coded here.

 From a user contributed 5800L /proc/bus/usb entries.
 Seem to be the same as 5700L
*/ 

#define EPL_5800L_USB_EP_OUTBOUND  0x01
#define EPL_5800L_USB_EP_INBOUND   0x82

/* 
 Mythical replies from 5800l.

 The reply size seems to be simply related to the first
 byte of a 2-byte command, hence we just use the first
 byte to index the size. 

 Possibly not needed, as 5800l seems to be very
 similar to 5900l, and 5900l seems to work 
 uni-directionally. Just put here for reference.
*/

static int epl_5800l_reply_size[] = 
  {
    18, /* 0x00, job header                   */
    18, /* 0x01, job footer                   */ 
    18, /* 0x02, 1st level encapsulation      */
    18, /* 0x03, 1st level decapsulation      */
    18, /* 0x04, page header                  */
    18, /* 0x05, page footer                  */
    0 , /* 0x06, strip */
    0,  /* 0x07, not used */
    0, 0, 0, 0, 0, 0, 0, 0, /* 0x08 - 0x0f, not used */
    33, /* 0x10, beginning of job    */
    33, /* 0x11, middle of polling   */ 
    19, /* 0x12, polling             */
    20  /* 0x13, 2nd command of job  */   
  };

