/**
 * Copyright (c) 2003 Roberto Ragusa
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
  int e;

#ifdef EPL_DEBUG
  fprintf(stderr, "EPL job footer\n");
#endif

  ts=temp_string;
  if(epl_job_info->model == MODEL_5700L)
    {
      ts += sprintf(ts, "%c%c",
        0x01,
        0x00
	);
    }
  else if(epl_job_info->model == MODEL_5800L
          || epl_job_info->model == MODEL_5900L)
    {
      ts += sprintf(ts, "\x01d");
      ts += sprintf(ts, "2eps{I");
      ts += sprintf(ts, "%c%c",
        0x03,
        0x00
        );
      ts += sprintf(ts, "\x01d");
      ts += sprintf(ts, "2eps{I");
      ts += sprintf(ts, "%c%c",
        0x01, /* 1=last page 2=next page??? maybe not */
        0x00
        );
      ts += sprintf(ts, "\x01b\x001");
      ts += sprintf(ts, "@EJL \x00a");
 
      if(epl_job_info->model == MODEL_5900L)
        {
	  ts += sprintf(ts, "@EJL EJ \x00a");
          ts += sprintf(ts, "\x01b\x001");
          ts += sprintf(ts, "@EJL \x00a");
	}
    }

#ifdef EPL_DEBUG
  fprintf(stderr, "Writing %s job footer (%i bytes)\n",
          printername[epl_job_info->model], ts - temp_string);
#endif
  e = fwrite(temp_string, 1, ts - temp_string, epl_job_info->outfile);
  if(e != ts - temp_string) return -1;

  return 0;
} 

