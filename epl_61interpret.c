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
   This routine interprets the reply for the 6100L.
   We only understand a few parameters, but we can get the info we need
   to slow down data transfer to avoid printer congestion.
*/
void epl_61interpret(EPL_job_info *epl_job_info, unsigned char *p, int len)
{
  int free_memory;
  int total_memory = -1; /* sentinel value */
 
  fprintf(stderr, "Printer says:\n");
  if (len < 8)
    {
      /* This condition should never happen */
      fprintf(stderr, "**Reply size too short - This condition should never happen.");
      return;
    }
  
  free_memory = (p[0x05] << 16) | (p[0x06] << 8) | p[0x07];
  fprintf(stderr, "  free memory        = 0x%8.8x\n", free_memory);
  
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
  
  /* finally, reset the *_after_last_update counters */
  epl_job_info->bytes_sent_after_last_update = 0;
  epl_job_info->stripes_sent_after_last_update = 0;
  
  return;
}
