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

/* we need more than -ansi to use the nanosleep routine */
#define _POSIX_C_SOURCE 199309L
#include <time.h>

#include <stdlib.h>

#include "epl_job.h"
#include "epl_bid.h"

/* sleep */
void sleep_milliseconds(int ms);

/*
   This routine interprets the reply for the 5800L and 5900L.
   We can see several parameters and printer condition.
   Some of them tell us to slow down data transfer to avoid printer
   congestion.
*/
void epl_59interpret(EPL_job_info *epl_job_info, unsigned char *p, int len, int code)
{
  int free_memory;
  static int last_waited_milliseconds = 1;
  
  if (len < 18)
    {
      /* This condition should never happen */
      fprintf(stderr, "**Reply size too short - This condition should never happen.");
    }
  if (len != 18 + p[0x11])
    {
      /* This condition should never happen */
      fprintf(stderr, "**Reply size inconsistent - This condition should never happen.");
    }
  
  fprintf(stderr, "Printer tells:\n");
  free_memory = (p[0x04] << 16) | (p[0x05] << 8) | p[0x06];
  fprintf(stderr, "  free memory      = 0x%8.8x\n", free_memory);
  
  fprintf(stderr, "  status(0x07)     = 0x%2.2x:",p[0x07]);
  if (p[0x07] & 0x80) fprintf(stderr, " UNKNOWN_80");
  if (p[0x07] & 0x40) fprintf(stderr, " UNKNOWN_40");
  if (p[0x07] & 0x20) fprintf(stderr, " UNKNOWN_20");
  if (p[0x07] & 0x10) fprintf(stderr, " JOB_PENDING");
  if (p[0x07] & 0x08) fprintf(stderr, " UNKNOWN_08");
  if (p[0x07] & 0x04) fprintf(stderr, " ENERGY_SAVING");
  if (p[0x07] & 0x02) fprintf(stderr, " WARMING_UP");
  if (p[0x07] & 0x01) fprintf(stderr, " JOB_FINISHED?");
  fprintf(stderr, "\n");
  
  fprintf(stderr, "  status(0x09)     = 0x%2.2x:",p[0x09]);
  if (p[0x09]&0x80) fprintf(stderr, " UNKNOWN_80");
  if (p[0x09]&0x40) fprintf(stderr, " UNKNOWN_40");
  if (p[0x09]&0x20) fprintf(stderr, " UNKNOWN_20");
  if (p[0x09]&0x10) fprintf(stderr, " UNKNOWN_10");
  if (p[0x09]&0x08) fprintf(stderr, " UNKNOWN_08");
  if (p[0x09]&0x04) fprintf(stderr, " OPEN_COVER");
  if (p[0x09]&0x02) fprintf(stderr, " NO_PAPER");
  if (p[0x09]&0x01) fprintf(stderr, " UNKNOWN_01");
  fprintf(stderr, "\n");
  
  fprintf(stderr, "  pages just print = %i\n", (p[0x0d] << 8) | p[0x0e]);
  
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
      fprintf(stderr, "  connection by    = %s\n", p[0x1b] & 0x02 ? "USB" : "PARPORT");
      fprintf(stderr, "  installed memory = %iMiB\n", p[0x1c]);
    }
  else if (p[0x11] == 0x11)
    {
      fprintf(stderr, "0x11 extension:\n");
      fprintf(stderr, "  printed pages    = %i\n", (p[0x19] << 8) | p[0x1a]);
      fprintf(stderr, "  toner supply     = %i%%\n", p[0x1b]);
      fprintf(stderr, "  imaging supply   = %i%%\n", p[0x1c]);
      fprintf(stderr, "  paper supply     = %i%%\n", p[0x1d]);
    }
  else
    {
      fprintf(stderr, "unknown 0x%2.2x extension, ignoring\n", p[0x11]);
    }
  
  fprintf(stderr, "updating freemem estimate from 0x%8.8x",
	  epl_job_info->estimated_free_mem);
  epl_job_info->estimated_free_mem = free_memory;
  fprintf(stderr, " to 0x%8.8x\n", epl_job_info->estimated_free_mem);
  
  /* If the buffer is almost full, go to sleep.
     Increase sleeping time if this condition persists.
     So, calling this function continuosly is not a problem.
     --  rora
  */
  if (free_memory < FREE_MEM_LOW_LEVEL)
    {
      int ms;
      ms = last_waited_milliseconds * 2; /* exponential back off */
      if (ms > 5000 ) ms = 5000; /* no more than five second */
      sleep_milliseconds(ms);
      last_waited_milliseconds = ms;
    }
  else
    {
      last_waited_milliseconds = 1;
    }
  return;
}

/*
   sleep for (at least) some milliseconds
*/
void sleep_milliseconds(int ms)
{
  struct timespec ts;

  fprintf(stderr, "sleeping %i milliseconds\n", ms);
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = 1000000 * (ms % 1000);
  nanosleep(&ts, NULL);
}

