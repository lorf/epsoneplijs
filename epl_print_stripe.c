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

int epl_print_stripe(EPL_job_info *epl_job_info, typ_stream *stream, int stripe_number) 
{
  char temp_string[256];
  char *ts;
  int count;
  int e;

#ifdef EPL_DEBUG
  fprintf(stderr, "EPL print stripe\n");
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
  else if(epl_job_info->model == MODEL_6100L) 
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
  fprintf(stderr, "Writing %s stripe header (%i bytes)\n",
          printername[epl_job_info->model], ts - temp_string);
#endif

  e = epl_write_uni(epl_job_info, temp_string, ts - temp_string);
  if(e != ts - temp_string) return -1;

  /* model-independent part - dump the stream body */

  e = epl_write_uni(epl_job_info, stream->start, count);
  if(e != count) return -1;

  return 0;
} 
