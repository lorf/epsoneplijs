/*
  Copyright (c) 2004 Hin-Tak Leung
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

#include <stdlib.h>

#include "epl_job.h"
#include "epl_bid.h"


/*
   This routine interprets the reply for the 6200L.
*/
void epl_62interpret(EPL_job_info *epl_job_info, unsigned char *reply, int reply_len)
{
  signed int free_memory  = -1;
 
  if (reply_len < 16)
    {
      /* This condition should never happen */
      fprintf(stderr, "**Reply size too short - This condition should never happen.\n");
      return;
    }

  if (reply_len != epl_bid_reply_len(EPL_6200L, reply[0]))
    {
      fprintf(stderr, "**Reply size don't agree with our knowledge\n");
      return;
    }
  
  /* 
  reply[0] is echoing the send command 
  reply[1] is the recent command seq number

  reply[2] is never seen other than 0 (probably higher byte of reply[3]
  reply[3] seems to be related to reply[0]
   P->7A   (0111 1010)
   Q->74   (0111 1010)
   R->0E   (0000 1110)
   @,O,B,7F,F,G,C,A,q,c,`->0C (0000 1100)
      Look like it is just a byte-to-follow count. 
  */
  if (reply[3] + 4 != reply_len)
    {
      fprintf(stderr, "**Reply size don't agree with embedded parameter length.\n");
      return;
    }
  /*
  reply[4] is never seen other than 0 (probably higher highy of reply[5,6,7]
  */
  free_memory = (reply[0x05] << 16) | (reply[0x06] << 8) | reply[0x07];
  fprintf(stderr, "  available memory        = 0x%8.8x\n", free_memory);
  
  fprintf(stderr, "updating last value 0x%8.8x\n", epl_job_info->free_mem_last_update);
  epl_job_info->free_mem_last_update = free_memory;

  /* if the printer is reporting more free memory than what we thought the total memory
     was, we can increase total_mem */
  if (epl_job_info->printer_total_mem < free_memory)
    {
      epl_job_info->printer_total_mem = free_memory;
    }
  
  /* finally, reset the *_after_last_update counters */
  epl_job_info->bytes_sent_after_last_update = 0;
  epl_job_info->stripes_sent_after_last_update = 0;

  switch(reply[0])
    {
    case '@':
    case 'A':
    case 'B':
    case 'C':
    case 'F':
    case 'G':
    case 'O':
    case 'q':
    case 'c':
    case '`':
    case 0x7F:
      break;

    case 'p':
      /* next 64 bytes from reply[24]  is client ID */
      /* next  4 bytes from reply[88]  is pages ever printed */
      /*       1 byte    at reply[101] is toner level */
      /*       1 byte    at reply[104] is photounit life */
      /*       1 byte    at reply[108] is epson code to paper size */
      break;

    case 'Q':
      /* 2 bytes       reply[54,55] is epson MC number */
      /* 20 bytes from reply[68]    is serial number - null padded */
      /* 32 bytes from reply[88]    is model number  - null padded */
      break;

    case 'R':
      break;

    default:
      break;
    }
  
  return;
}
