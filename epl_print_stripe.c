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
#include <unistd.h>
#include "epl_job.h"

int stripe_data_too_big(EPL_job_info *epl_job_info, typ_stream *stream, int stripe_number, int from_top);

#define ABS(x) (((x) > 0)? (x): -(x)) 

/* 1 if we won't be able to finish this page after sending this stripe */
int stripe_data_too_big(EPL_job_info *epl_job_info, typ_stream *stream, int stripe_number, int from_top)
{
  int ret = 0;
  int total_stripe = (epl_job_info->pixel_v + 64 - 1) / 64;
  
  /* HTL: 5700L has a 48 byte per stripe over head, 5900L has a 64 byte overhead;
     my guess for the 6100L/6200L would be larger or same as 5900L;
     + small per-job overhead - it seems to fluctuate by 30k */
    int min_needed_after_this_stripe = (104 + ((epl_job_info->model == MODEL_5700L) ? 48 : 64))
      * (total_stripe - stripe_number) + 80000;
    int expected_to_remain_after_this_stripe = 
      epl_job_info->free_mem_last_update 
      - (from_top ? 
	 (epl_job_info->bytes_sent_this_page + stripe_number * ((epl_job_info->model == MODEL_5700L) ? 48 : 64))
	 : 0)
      -  (stream->count + ((epl_job_info->model == MODEL_5700L) ? 48 : 64));

#ifdef EPL_DEBUG
    fprintf(stderr, "Need %i, Expect %i remain after stripe #%i\n", 
	    min_needed_after_this_stripe,
	    expected_to_remain_after_this_stripe,
	    stripe_number);
#endif
    
    if (min_needed_after_this_stripe > expected_to_remain_after_this_stripe)
      ret = 1;
    
    if (epl_job_info->model == MODEL_5700L)
      {
	/* HTL: I do not understand this, but it seems to be a genuine limit per page */
	if ((epl_job_info->bytes_sent_this_page + stream->count + 104 * (total_stripe - stripe_number)) > 2000000)
	  ret = 1;
      }
    
    if(ret)
      fprintf(stderr, "blanking after stripe %i, %i bytes\n", 
	      epl_job_info->stripes_sent_this_page, epl_job_info->bytes_sent_this_page); 
    
    return ret;
}

int epl_print_stripe(EPL_job_info *epl_job_info, typ_stream *stream, int stripe_number)
{
  char temp_string[256];
  char *ts;
  int count;
  int e;

#ifdef EPL_DEBUG
  fprintf(stderr, "EPL print stripe\n");
#endif
  
  if (stripe_data_too_big(epl_job_info, stream, stripe_number, !epl_job_info->paused_mid_page))
    {
      int old_free_mem = -12000 ; /* just to be far away from the real value */
      epl_poll(epl_job_info,2);
      if(!(epl_job_info->paused_mid_page))
	{
	  /* smallest full page print job is 6k? */
	  while (ABS(old_free_mem - epl_job_info->free_mem_last_update) > 6000) 
	    {
	      epl_job_info->paused_mid_page = 1;
	      sleep(10); /* 10 sec should be enough for any cached full-page to come out */
	      old_free_mem = epl_job_info->free_mem_last_update;
	      epl_poll(epl_job_info,2);
	    } /* when we get out of the while loop, free_mem had reached a not-changing value */
	}
      /* we just polled, now we need to decide */
      if (stripe_data_too_big(epl_job_info, stream, stripe_number, !epl_job_info->paused_mid_page))
	{
	  fprintf(stderr, "Stripe %i with %i bytes is being replaced with blank.\n",
		  stripe_number, stream->count);
#ifdef EPL_DEBUG
	  fprintf(stderr, "Expect %i next\n",  epl_job_info->free_mem_last_update
		  - 104 - ((epl_job_info->model == MODEL_5700L) ? 48 : 64));
#endif
#ifdef PRINT_AS_MUCH_AS_POSSIBLE_ON_LOW_MEMORY
	  if(epl_job_info->paused_mid_page == 1)
	    {
	      epl_job_info->paused_mid_page = 2;
	      make_lowmem_msg(stream);
	    }
	  else
	    {
	      make_blank(stream);
	    }
#endif
	}
    }

  (epl_job_info->stripes_sent_this_page)++;
  (epl_job_info->bytes_sent_this_page) += stream->count;
#ifdef EPL_DEBUG
  fprintf(stderr, "to do stripe %i, and up to %i bytes\n", epl_job_info->stripes_sent_this_page, epl_job_info->bytes_sent_this_page); 
#endif
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
