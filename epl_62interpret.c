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

/* poorman's binary strncmp */
static int bstrncmp(unsigned char *src, unsigned char *dest, int len) {
  int i, ret = 0;
  for(i = 0; i < len ; i++)
    {
      if (*(src +i) != *(dest + i))
	{
	  ret = 1;
	break;
	}
    }
  return ret;
}

static void hex_dump(char *marker, unsigned IDX, unsigned count, unsigned char *HEX) {
  int i;
    fprintf(stderr,"%s:%u:", marker, IDX);
    for(i = 0; i < count ; i++)
      {
	fprintf(stderr," %2.2X", HEX[i+IDX]);
      }
    fprintf(stderr,"\n");  
}

static void hex_dump_notequal(char *marker, unsigned char *std, int offset, int length, unsigned char *reply)
{
  if(bstrncmp(std, reply + offset, length))
    {
      hex_dump(marker, offset, length, reply);
    }
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

  hex_non_zero(2, reply);
  if (reply[3] + 4 != reply_len)
    {
      fprintf(stderr, "**Reply size don't agree with embedded parameter length.\n");
      return;
    }
  /*
  reply[4] is never seen other than 0 (probably higher highy of reply[5,6,7]
  */

  hex_non_zero(4, reply);
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
    fprintf(stderr, "Processing...\n");
  else
    hex_non_zero(10,reply);

  switch(reply[11])
    {
    case 0x01:
      fprintf(stderr, "Power - Ready\n");
      break;
    case 0x00:
      fprintf(stderr, "Power - Standby\n");
      break;
    default:
      fprintf(stderr, "Unknown power state %2.2X\n", reply[122]);
      break;
    }

  hex_non_zero(12,reply);

  if(reply[13])
    fprintf(stderr, "printed %i pages since power up\n", reply[13]);

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
      /* nominally 16 */
    case '@': 
    case 'A':
    case 'B':
    case 'C':
    case 'F':
    case 'G':
    case 'O':
    case 'q': /* very strange, only seems to be issued when out-of-paper is detected. */
    case 'c': /* send "c 01 00 64" to reset OPC level to 100%(0x64) */
    case '`': /* "` 01 00 00 00 00 00 00" disable sleep */
              /* "` 01 00 02 00 00 00 00" enable  sleep */
              /* "` 01 03 01 00 00 00 00" toner out stop */
              /* "` 01 03 00 00 00 00 00" toner out cont */
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
      {
	unsigned char P0[] = {0x51, 0xB2};
	hex_dump_notequal("P0", P0, 16, 2,reply);
      }
      hex_dump("P1", 18, 6,reply);
      fprintf(stderr, "Client ID: \"");
      fwrite(reply+24, 1, 64, stderr);
      fprintf(stderr, "\"\n");
      fprintf(stderr, "Page ever printed: %u\n",
	      ((((((reply[88] << 8) + reply[89]) << 8) + reply[90]) << 8) + reply[91]));
      {
	unsigned char P2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff};
	hex_dump_notequal("P2", P2, 92, 9, reply);
      }
      fprintf(stderr, "Toner Level: %u\n", reply[101]);
      {
	unsigned char P3[] = {0xff, 0xff};
	hex_dump_notequal("P3", P3, 102, 2, reply);
      }
      fprintf(stderr, "Photounit life: %u\n", reply[104]);
      {
	unsigned char P4[] = {0xff, 0xff, 0xff};
	hex_dump_notequal("P4", P4, 105, 3, reply);
      }
      fprintf(stderr, "Paper size code: %2.2X\n", reply[108]);
      {
	unsigned char P5[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	hex_dump_notequal("P5", P5, 109, 11, reply);
      }
      if(reply[120])
	fprintf(stderr, "Time to sleep: %i minutes\n", reply[120] * 10);
      else
	fprintf(stderr, "Sleep disabled\n");	
      {
	unsigned char P6[] = {0x00};
	hex_dump_notequal("P6", P6, 121, 1, reply);
      }
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

      {
	unsigned char P5[] = {0x01, 0x00, 0x00};
	hex_dump_notequal("P5", P5, 123, 3, reply);
      }
      break;

    case 'Q': /* 120 */
      /* 2 bytes       reply[54,55] is epson MC number */
      /* 20 bytes from reply[68]    is serial number - null padded */
      /* 32 bytes from reply[88]    is model number  - null padded */
      {
	unsigned char Q1[] = 
	  {0x51, 0xB2, 0x00, 0x02, 0x00, 0x1E, 0x00, 0x00, 0x01, 0x31, 0xA4, 0x9C, 0x02, 0x02, 0x0F, 0x00,
	   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x0A, 0x00, 0x01, 0x02, 0x00, 0x00,
	   0x01, 0x01, 0x00, 0x00, 0x00, 0x00};    
	hex_dump_notequal("Q1", Q1, 16, 38,reply);
      }
      fprintf(stderr, "Epson MC %u\n", (reply[54] << 8) + reply[55]);
      {
	unsigned char Q2[] = 
	  {0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00};
	hex_dump_notequal("Q2", Q2, 56, 12,reply);
      }
      reply[87] = '\0'; /* terminates string */
      fprintf(stderr, "Serial Number: %s\n", reply + 68); 
      reply[119] = '\0';/* terminates string */
      fprintf(stderr, "Serial Number: %s\n", reply + 88); 
      break;

    case 'R': /* 18 */ /* request for the next page number? - just before job header */
      /* 00 + page number of the next page - before page header */
      hex_dump("R1", 16, 2, reply);
      break;

    default:
      fprintf(stderr, "Should not got here\n"); 
#ifdef ABORT
      exit(1);
#endif
      break;
    }
  
  return;
}

/* 
PQP every 10 seconds - very curious; sometimes 5 seconds; while idle.

R,P@P,PPQ,POB..... in a job.

7f every 0.5 sec, for 5s, then a P, etc during a poll for paper-out/cover-open.

*/
