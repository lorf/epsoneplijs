/**
 * Copyright (c) 2003 Hin-Tak Leung, Roberto Ragusa
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

#include <stdio.h>
#include "epl_job.h"

/* put here for efficiency */
static unsigned char blank_stripe[] = {
  0xa0, 0x1d, 0x74, 0x03, 0x0e, 0x80, 0x01, 0xd0, 0x40, 0x3a, 0xe8, 0x07, 0x1d, 0x00, 0x03, 0xa0, 
  0x80, 0x74, 0xd0, 0x0e, 0x3a, 0x01, 0x07, 0x40, 0x00, 0xe8,
  0xa0, 0x1d, 0x74, 0x03, 0x0e, 0x80, 0x01, 0xd0, 0x40, 0x3a, 0xe8, 0x07, 0x1d, 0x00, 0x03, 0xa0, 
  0x80, 0x74, 0xd0, 0x0e, 0x3a, 0x01, 0x07, 0x40, 0x00, 0xe8,
  0xa0, 0x1d, 0x74, 0x03, 0x0e, 0x80, 0x01, 0xd0, 0x40, 0x3a, 0xe8, 0x07, 0x1d, 0x00, 0x03, 0xa0, 
  0x80, 0x74, 0xd0, 0x0e, 0x3a, 0x01, 0x07, 0x40, 0x00, 0xe8,
  0xa0, 0x1d, 0x74, 0x03, 0x0e, 0x80, 0x01, 0xd0, 0x40, 0x3a, 0xe8, 0x07, 0x1d, 0x00, 0x03, 0xa0, 
  0x80, 0x74, 0xd0, 0x0e, 0x3a, 0x01, 0x07, 0x40, 0x00, 0xe8,
};


int epl_print_stripe(EPL_job_info *epl_job_info, typ_stream *stream, int stripe_number)
{
  char temp_string[256];
  char *ts;
  int count;
  int e;
  int total_stripe = (epl_job_info->pixel_v + 64 - 1) / 64;

#ifdef EPL_DEBUG
  fprintf(stderr, "EPL print stripe\n");
#endif
  (epl_job_info->stripes_sent_this_page)++;
  (epl_job_info->bytes_sent_this_page) += stream->count;
  
  if (104 * (total_stripe - stripe_number) >
      (epl_job_info->free_mem_last_update - epl_job_info->bytes_sent_this_page))
    {
      int idx = 0;
      fprintf(stderr, "Stripe %i is going critical\n", stripe_number);
      /* it may be wise to poll here, but then, it may be not */
#ifdef PRINT_AS_MUCH_AS_POSSIBLE_ON_LOW_MEMORY
      stream->count = 104;
      for (idx =0 ; idx < 104; idx++)
	{
      *(stream->start + idx) = blank_stripe[idx];
	}
#endif
    }
  /* this part does the model-dependent stripe header */

  ts = temp_string;
  count = stream->count;

  if(epl_job_info->model == MODEL_5700L) 
    {
      ts += sprintf(ts, "%c%c%c%c%c%c%c",
        0x04,
	0x00,
	0x01,
        count >> 24, count >> 16, count >> 8, count
	);
    }
  
  else if(epl_job_info->model == MODEL_5800L
          || epl_job_info->model == MODEL_5900L) 
    {
      ts += epl_sprintf_wrap(ts, count + 7);
      ts += sprintf(ts, "%c%c%c%c%c%c%c",
	0x06,
	0x00,
	0x01,
	count >> 24, count >> 16, count >> 8, count
	);
    }
  else if((epl_job_info->model == MODEL_6100L) || (epl_job_info->model == MODEL_6200L)) 
    {
      ts += epl_sprintf_wrap(ts, count + 12);
      ts += sprintf(ts, "L%c%c%c%c%c%c%c%c%c%c%c",
		    0x00, 0x01, 0x04, 0x00,
		    stripe_number,
		    0x00, 0x00,
		    count >> 24, count >> 16, count >> 8, count
		    );
    }
  
#ifdef EPL_DEBUG
  fprintf(stderr, "Writing %s stripe header (%i bytes)",
          printername[epl_job_info->model], ts - temp_string);
#endif

  e = epl_write_uni(epl_job_info, temp_string, ts - temp_string);
  if(e != ts - temp_string) return -1;

  /* model-independent part - dump the stream body */

#ifdef EPL_DEBUG
  fprintf(stderr, " and stripe data (%i bytes)\n", count);
#endif

  e = epl_write_uni(epl_job_info, stream->start, count);
  if(e != count) return -1;

  return 0;
} 
