/*
  Copyright (c) 2003 Roberto Ragusa
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
   This routine interprets the reply for the 5800L and 5900L.
   We can see several parameters and printer condition.
   Some of them tell us to slow down data transfer to avoid printer
   congestion.
*/
void epl_59interpret(EPL_job_info *epl_job_info, unsigned char *p, int len)
{
  int free_memory;
  int total_memory = -1; /* sentinel value */
  
  fprintf(stderr, "Printer says:\n");
  if (len < 18)
    {
      /* This condition should never happen */
      fprintf(stderr, "**Reply size too short - This condition should never happen.");
      return;
    }
  if (len != 18 + p[0x11])
    {
      /* This condition should never happen */
      fprintf(stderr, "**Reply size inconsistent - This condition should never happen.");
      return;
    }
  
  free_memory = (p[0x04] << 16) | (p[0x05] << 8) | p[0x06];
  fprintf(stderr, "  free memory        = 0x%8.8x\n", free_memory);
  
  fprintf(stderr, "  status(0x07)       = 0x%2.2x:",p[0x07]);
  if (p[0x07] & 0x80) fprintf(stderr, " UNKNOWN_80");
  if (p[0x07] & 0x40) fprintf(stderr, " UNKNOWN_40");
  if (p[0x07] & 0x20) fprintf(stderr, " UNKNOWN_20");
  if (p[0x07] & 0x10) fprintf(stderr, " JOB_PENDING");
  if (p[0x07] & 0x08) fprintf(stderr, " UNKNOWN_08");
  if (p[0x07] & 0x04) fprintf(stderr, " ENERGY_SAVING");
  if (p[0x07] & 0x02) fprintf(stderr, " WARMING_UP");
  if (p[0x07] & 0x01) fprintf(stderr, " JOB_FINISHED?");
  fprintf(stderr, "\n");
  
  fprintf(stderr, "  status(0x09)       = 0x%2.2x:",p[0x09]);
  if (p[0x09] & 0x80) fprintf(stderr, " UNKNOWN_80");
  if (p[0x09] & 0x40) fprintf(stderr, " UNKNOWN_40");
  if (p[0x09] & 0x20) fprintf(stderr, " UNKNOWN_20");
  if (p[0x09] & 0x10) fprintf(stderr, " UNKNOWN_10");
  if (p[0x09] & 0x08) fprintf(stderr, " UNKNOWN_08");
  if (p[0x09] & 0x04) fprintf(stderr, " OPEN_COVER");
  if (p[0x09] & 0x02) fprintf(stderr, " NO_PAPER");
  if (p[0x09] & 0x01) fprintf(stderr, " UNKNOWN_01");
  fprintf(stderr, "\n");
  
  fprintf(stderr, "  pages just printed = %i\n", (p[0x0d] << 8) | p[0x0e]);
  
  if (p[0x11] == 0)
    {
      fprintf(stderr, "no extension\n");
    }
  else if (p[0x11] == 0x01)
    {
      fprintf(stderr, "0x01 extension:\n");
    }
  else if (p[0x11] == 0x0f)
    {
      fprintf(stderr, "0x0f extension:\n");
      switch(p[0])
	{
	case 0x10: /* 5800L, 5900L */
          fprintf(stderr, "(of 0x10 type)\n");
	  fprintf(stderr, "  connection by      = %s\n", p[0x1b] & 0x02 ? "USB" : "PARPORT");
	  fprintf(stderr, "  installed memory   = %iMiB\n", p[0x1c]);
	  total_memory = p[0x1c] * 1048576;
	  break;
	case 0x11: /* 5800L only */
          fprintf(stderr, "(of 0x11 type)\n");
	  fprintf(stderr, "  printed pages      = %i\n", (p[0x19] << 8) | p[0x1a]);
	  fprintf(stderr, "  toner supply       = %i%%\n", p[0x1b]);
	  fprintf(stderr, "  imaging supply     = %i%%\n", p[0x1c]);
	  fprintf(stderr, "  paper supply       = %i%%\n", p[0x1d]);
	  break;
	default:
	  break;
	}
    }
  else if (p[0x11] == 0x11) /* 5900L only */
    {
      fprintf(stderr, "0x11 extension:\n");
      fprintf(stderr, "  standby            = %i (%s)\n", p[0x12], p[0x12] ? "enabled" : "disabled");
      fprintf(stderr, "  printed pages      = %i\n", (p[0x19] << 8) | p[0x1a]);
      fprintf(stderr, "  toner supply       = %i%%\n", p[0x1b]);
      fprintf(stderr, "  imaging supply     = %i%%\n", p[0x1c]);
      fprintf(stderr, "  paper supply       = %i%%\n", p[0x1d]);
      fprintf(stderr, "  low toner behav.   = %i (%s)\n", p[0x22], p[0x22] ? "stop" : "go on");
    }
  else
    {
      fprintf(stderr, "unknown 0x%2.2x extension, ignoring\n", p[0x11]);
    }
  
  fprintf(stderr, "updating free_mem from 0x%8.8x",
	  epl_job_info->free_mem_last_update);
  epl_job_info->free_mem_last_update = free_memory;
  fprintf(stderr, " to 0x%8.8x\n",
          epl_job_info->free_mem_last_update);

  /* if the printer is reporting more free memory than what we thought the total memory
     was, we can increase total_mem */
  if (epl_job_info->printer_total_mem < free_memory) total_memory = free_memory;
  /* do we have to update total_mem? */
  if (total_memory != -1)      /* we got the number */
    {
      fprintf(stderr, "updating total_mem from 0x%8.8x",
	      epl_job_info->printer_total_mem);
      epl_job_info->printer_total_mem = total_memory;
      fprintf(stderr, " to 0x%8.8x\n",
              epl_job_info->printer_total_mem);
    }
  return;
}
