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

#include <unistd.h>
#include <stdlib.h>

#include "epl_job.h"
#include "epl_bid.h"
#include "epl_time.h"

/* forward declarations */
void epl_57_dump_extra(int length, unsigned char *buffer);


/*
  This routine is completely 5700L-specific: It interpretes
  the USB replies from the printer to work out if there was a paper
  jam, etc.
*/

void epl_57interpret(unsigned char *buffer, int len)
{
  long free_memory;
  long page_printed;
  /* 
     0 to terminate error conditions. 
     - length of "abort_if_not_zero" may change in the future 
     - This utilise a convenient fact that position 0 is always 
     just reflecting the sent command, and is neither informative
     nor abortive.
  */
  int abort_if_not_zero[] = {2,3,
			     8,10,11,12,
			     0}; 
  int i_abort = 0;

#ifdef EPL_DEBUG
  int i_ret;
  fprintf(stderr, "Printer replied: ");
  for (i_ret = 0 ; i_ret < epl_bid_reply_len(MODEL_5700L, (int) *buffer) ; i_ret++)
    {
      fprintf(stderr,"%2.2X ", (0xff & buffer[i_ret])); 
    }
  fprintf(stderr, "\n");
#endif
  
  if (len < 15) 
    {
      /* This condition should never happen */
      fprintf(stderr, "**Short reply size - This condition should never happen.");
    }

  if (epl_bid_reply_len(MODEL_5700L, (int) *buffer) - (int) buffer[14] != 15) 
    {
      /* This condition should never happen */
      fprintf(stderr, "**Inconsistent reply size - This condition should never happen.");
    }
  
  /* Information first */
  
  free_memory = (buffer[4]<<16) + (buffer[5]<<8) + buffer[6];
  fprintf(stderr, "Mem Free \t=%8ld\tStatus\t=%2.2X/%2.2X\n", 
	  free_memory,
	  (0xff & buffer[7]) , /* Readiness Flag */
	  (0xff & buffer[13])  /* Fault Count */
	  ); 
  /* Readiness = 0x04 during powersaving mode ; deeper sleep to 0x05 */

  switch (buffer[14])
    {
    case 0:  /* standard 00-04: */
      break;
      
    case 6:  /* 05: 00 1F FF F8 00 30 */
      if ((buffer[15] != 0x00) ||
	  (buffer[16] != 0x1F) ||
	  ((0xff & buffer[17]) != 0xFF) ||
	  ((0xff & buffer[18]) != 0xF8) ||
	  (buffer[19] != 0x00) ||
	  (buffer[20] != 0x30)) 
	{
	  epl_57_dump_extra(buffer[14], &buffer[15]);
	}
      break;

    case 4:  /* 06: 02 03 FE 00 */
      if (((0xff & buffer[17]) != 0xFE) ||
	  (buffer[18] != 0x00))
	{
	  epl_57_dump_extra(buffer[14], &buffer[15]);
	}
      switch (buffer[15])
	{
	case 0x00: 
	  fprintf(stderr, "Connected: Parport\n");
	  break;
	case 0x02: 
	  fprintf(stderr, "Connected: USB\n");
	  break;
	default:
	  fprintf(stderr, "Connected: Unknown %2.2X\n", (0xff & buffer[15]));
	  break;
	}
      break;
      switch (buffer[16])
	{
	case 0x00: 
	  fprintf(stderr, "Standby Mode Disabled\n");
	  break;
	default:
	  fprintf(stderr, "Standby Mode after idle for %d minutes\n", buffer[16] * 10);
	  break;
	}
      break;

    case 13: /* 07: 01 31 08 B8 02 00 00 + 6 bytes */
      if ((buffer[15] != 0x01) ||
	  (buffer[16] != 0x31) ||
	  (buffer[17] != 0x08) ||
	  ((0xff & buffer[18]) != 0xB8) ||
	  (buffer[19] != 0x02) ||
	  (buffer[20] != 0x00) ||
	  (buffer[21] != 0x00))
	{
	  /* Have never seen this part being different either */
	  epl_57_dump_extra(7, &buffer[15]);
	}
      page_printed = (buffer[22]<<8) + buffer[23];
      fprintf(stderr, "Pages Printed\t=%li\n", page_printed); 
      fprintf(stderr, "Toner Level \t=%i%%\n", buffer[24]);
      fprintf(stderr, "PhotoConductor Life\t=%i%%\n", buffer[25]);
      if ((0xff & buffer[26]) != 0xff) {
      fprintf(stderr, "Tray Paper Level \t=%i%%\n", buffer[26]);
      } else {
      fprintf(stderr, "Tray missing\t: error %2.2X\n", buffer[26]);
      }
	
      if ((0xff & buffer[27]) != 0xff) {
	fprintf(stderr, "Cassette Paper Level\t=%i%%\n", buffer[27]);
      }
      break;

    case 2: /* 8 */
      if ((buffer[15] != 0x00) ||
	  (buffer[16] != 0x00))
	{
	  epl_57_dump_extra(buffer[14], &buffer[15]);
	}
      break;
    default:
      epl_57_dump_extra(len, buffer);
      break;
    }

  /* Pause condition Afterwards */
  
  if (buffer[7] == 0x11) {
    /* don't know how long to sleep, but the printer said "please back off" */
    sleep_seconds(2); 
  } else if (buffer[7] > 0x11) {
    fprintf(stderr,"Probably should abort here for pos[7] = %2.2X\n", (0xff & buffer[7]));
    sleep_seconds(2); 
  }

  if (buffer[1] > 0x00) {
    fprintf(stderr,"Pause on pos[1] = %2.2X\n", (0xff & buffer[1]));
    sleep_seconds(2); 
  }

  if(buffer[9] == 0x04) 
    {
      /* cover open = 0x04 */
      fprintf(stderr,"Cover open: %2.2X\n", (0xff & buffer[9]));
    } 
  else 
    if (buffer[9] == 0x02)
    {
      /* no paper = 0x02 */
      fprintf(stderr,"Out of paper: %2.2X\n", (0xff & buffer[9]));
    } 
  else 
    if(buffer[9] != 0x00) 
    {
      /* Probably should abort here */
      fprintf(stderr,"Pause on pos[9] = %2.2X\n", (0xff & buffer[9]));
      sleep_seconds(2); 
    }
  
  /* Abort condition Finally */

  if(buffer[10] != 0x00) 
    {
      fprintf(stderr,"Paper Jam: %2.2X\n", (0xff & buffer[10]));
    }

  while (abort_if_not_zero[i_abort] != 0 ) 
    {
      if (buffer[abort_if_not_zero[i_abort]] != 0x00 )
	{ 
	  fprintf(stderr,"Aborting at command %2.2X on pos[%d] = %2.2X\n",
		  (0xff & buffer[0]), 
		  abort_if_not_zero[i_abort], 
		  (0xff & buffer[abort_if_not_zero[i_abort]])
		  ); 
	  
	  fprintf(stderr,"The rest = ");
	  
	  while (abort_if_not_zero[i_abort] != 0 ) 
	    {
	      if (buffer[abort_if_not_zero[i_abort]] != 0) {
		fprintf(stderr,"<[%d]:%2.2X>",
			abort_if_not_zero[i_abort], 
			(0xff & buffer[abort_if_not_zero[i_abort]])
			); 
	      }
	      i_abort++ ;
	    }
	  
	  fprintf(stderr,"\n");

	  exit(1);
	}
      i_abort++ ;
    }
}

void epl_57_dump_extra(int length, unsigned char *buffer)
{
  int i;
  fprintf(stderr,"{"); 
  
  for (i = 0 ; i < length ; i++)
    {
      fprintf(stderr,"%2.2X ", (0xff & buffer[i])); 
    }
  fprintf(stderr,"}\n"); 
}
