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


static void hex_non_zero(unsigned IDX, unsigned char *HEX) {
  if(HEX[IDX]){
    fprintf(stderr,"%u:%2.2X\n", IDX, HEX[IDX]);  
  }
}

static void hex_dump(char *marker, unsigned IDX,unsigned count, unsigned char *HEX) {
  int i;
    fprintf(stderr,"%s:%u:", marker, IDX);
    for(i = 0; i < count ; i++)
      {
	fprintf(stderr," %2.2X", HEX[i+IDX]);
      }
    fprintf(stderr,"\n");  
}

/*
   This routine interprets the reply for the 6200L.
*/

void epl_62interpret(EPL_job_info *epl_job_info, unsigned char *reply, int reply_len)
{
  signed int free_memory  = -1;
 
  if (reply_len < 16)
    {
      /* This condition should never happen */
      fprintf(stderr, "**Reply size = %i too short - This condition should never happen.\n", reply_len);
      return;
    }

  if (reply_len != epl_bid_reply_len(MODEL_6200L, reply[0]))
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

  fprintf(stderr, "Sequence #:%u\n", reply[1]);
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
  
  if(reply[8] == 0x02) 
    fprintf(stderr, "NO PAPER\n");
  else
    hex_non_zero( 8,reply);

  if(reply[9] == 0x01)
    fprintf(stderr, "Cover open\n");
  else
    hex_non_zero( 9,reply);

  if(reply[10] == 0x01)
    fprintf(stderr, "Receiving Data...\n");
  else
    hex_non_zero(10,reply);

  hex_non_zero(11,reply);
  hex_non_zero(12,reply);

  if(reply[13]) fprintf(stderr, "printed %i pages since power up\n", reply[13]);

  hex_non_zero(14,reply);

  if(reply[15]) fprintf(stderr, "page %i in progress\n", reply[15]);

  if(epl_job_info)
    {
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
    }
  switch(reply[0])
    {
    case '@': /* nominally 16 */
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

    case 'P': /* 126 */
      /* next 64 bytes from reply[24]  is client ID */
      /* next  4 bytes from reply[88]  is pages ever printed */
      /*       1 byte    at reply[101] is toner level */
      /*       1 byte    at reply[104] is photounit life */
      /*       1 byte    at reply[108] is epson code to paper size */
      /*
    190 P1:16: 51 B2 03 E8 01 00 FF FF
    216 P1:16: 51 B2 03 EA 02 00 00 00
      2 P1:16: 51 B2 03 EE 01 00 FF FF
      8 P1:16: 51 B2 03 EE 02 00 00 00
      7 P1:16: 51 B2 03 F1 04 00 00 00
     35 P1:16: 51 B2 03 F1 11 00 00 00
      9 P1:16: 51 B2 0F A2 01 00 FF FF
     19 P1:16: 51 B2 0F A2 02 00 00 00
     67 P1:16: 51 B2 0F AB 02 00 00 00
      3 P1:16: 51 B2 0F B9 01 00 FF FF
     19 P1:16: 51 B2 0F B9 02 00 00 00
      */
      hex_dump("P1", 16, 8,reply);
      fprintf(stderr, "Client ID: \"");
      fwrite(reply+24, 1, 64, stderr);
      fprintf(stderr, "\"\n");
      fprintf(stderr, "Page ever printed: %u\n",
	      ((((((reply[88] << 8) + reply[89]) << 8) + reply[90]) << 8) + reply[91]));
      hex_dump("P2", 92, 9, reply);
      fprintf(stderr, "Toner Level: %u\n", reply[101]);
      hex_dump("P3", 102, 2, reply);
      fprintf(stderr, "Photounit life: %u\n", reply[104]);
      hex_dump("P4", 105, 3, reply);
      fprintf(stderr, "Paper size code: %2.2X\n", reply[108]);
      /* 00 00 00 00 00 FF FF FF FF FF FF */
      hex_dump("P5", 109, 11, reply);

      if(reply[120])
	fprintf(stderr, "Time to sleep: %i minutes\n", reply[120]);
      else
	fprintf(stderr, "Sleep disabled\n");
	
      /* 00 */
      hex_dump("P6", 121, 1, reply);

      switch(reply[122])
	{
	case 0x01:
	  fprintf(stderr, "Toner out stop\n");
	  break;
	case 0x00:
	  fprintf(stderr, "Toner out continue\n");
	  break;
	default:
	  fprintf(stderr, "Unknown toner out state %2.2X\n", reply[122]);
	  break;
	}

      /* 01 00 00 */
      hex_dump("P5", 123, 3, reply);
      break;

    case 'Q': /* 120 */
      /* 2 bytes       reply[54,55] is epson MC number */
      /* 20 bytes from reply[68]    is serial number - null padded */
      /* 32 bytes from reply[88]    is model number  - null padded */
      /* Q1:16: 51 B2 00 02 00 1E 00 00  01 31 A4 9C 02 02 0F 00 
                00 00 00 00 00 00 00 00  0E 00 0A 00 01 02 00 00 
                01 01 00 00 00 00
      */
      hex_dump("Q1", 16, 38,reply);
      fprintf(stderr, "Epson MC %u\n", (reply[54] << 8) + reply[55]);
      /* Q2:56: 00 00 00 00 03 00 00 00 00 01 01 00 */
      hex_dump("Q2", 56, 12,reply);
      reply[87] = '\0'; /* terminates string */
      fprintf(stderr, "Serial Number: %s\n", reply + 68); 
      reply[119] = '\0';/* terminates string */
      fprintf(stderr, "Serial Number: %s\n", reply + 88); 
      break;

    case 'R': /* 18 */
      /* 00 count of some sort */
      hex_dump("R1", 16, 2, reply);
      break;

    default:
#ifdef ABORT
      exit(1);
#endif
      break;
    }
  
  return;
}
