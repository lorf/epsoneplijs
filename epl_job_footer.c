/**
 * Copyright (c) 2003 Roberto Ragusa, Hin-Tak Leung
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

int epl_job_footer(EPL_job_info *epl_job_info) 
{
  char temp_string[256];
  char *ts;
  int ts_count; /* how many strings */
  char *ts_beg[5]; /* where strings begin */
  int i, e;

#ifdef EPL_DEBUG
  fprintf(stderr, "EPL job footer\n");
#endif

  ts = temp_string;
  ts_count = 0;
  ts_beg[ts_count++] = ts;
  if(epl_job_info->model == MODEL_5700L)
    {
      ts += sprintf(ts, "%c%c",0x01,0x00);
      ts_beg[ts_count++] = ts;
    }
  else if(epl_job_info->model == MODEL_5800L
          || epl_job_info->model == MODEL_5900L)
    {
      ts += epl_sprintf_wrap(ts, 2);
      ts += sprintf(ts, "%c%c", 0x03, 0x00);
      ts_beg[ts_count++] = ts;
      ts += epl_sprintf_wrap(ts, 2);
      ts += sprintf(ts, "%c%c", 0x01, 0x00);
      ts_beg[ts_count++] = ts;
      
      ts += sprintf(ts, "\x1b\x01");
      ts += sprintf(ts, "@EJL \x0a");
      ts_beg[ts_count++] = ts;
      
      if(epl_job_info->model == MODEL_5900L)
        {
	  ts += sprintf(ts, "@EJL EJ \x0a");
          ts += sprintf(ts, "\x1b\x01");
          ts += sprintf(ts, "@EJL \x0a");
          ts_beg[ts_count++] = ts;
	}
    }
  else if(epl_job_info->model == MODEL_6100L)
    {
      ts += epl_sprintf_wrap(ts, 2);
      ts += sprintf(ts, "C%c", 0x00);
      ts_beg[ts_count++] = ts;
      ts += epl_sprintf_wrap(ts, 2);
      ts += sprintf(ts, "A%c", 0x00);
      ts_beg[ts_count++] = ts;

      ts += sprintf(ts, "\x1b\x01");
      ts += sprintf(ts, "@EJL \x0a");
 
      ts += sprintf(ts, "@EJL EJ \x0a");
      ts += sprintf(ts, "\x1b\x01");
      ts += sprintf(ts, "@EJL \x0a");
      ts_beg[ts_count++] = ts;
    }

#ifdef EPL_DEBUG
  fprintf(stderr, "Writing %s job footer (%i bytes in %i parts)\n",
          printername[epl_job_info->model], ts - temp_string, ts_count);
#endif

  for (i = 0 ; i < ts_count - 1 ; i++)
    {
      fprintf(stderr,"string %i from %p to %p\n", i, ts_beg[i], ts_beg[i+1]);
      e = epl_write_bid(epl_job_info, ts_beg[i], ts_beg[i+1] - ts_beg[i]);
      if(e != ts_beg[i+1] - ts_beg[i]) return -1;
    }

  return 0;
} 

