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

/*
  Type is 0, 1 or 2. This is mapped to hex codes in a printer specific way.
  Look at epl_usb_replies.h.
*/
int epl_poll(EPL_job_info *epl_job_info, int type)
{
  char temp_string[256];
  char *ts;
  int e;

#ifdef EPL_DEBUG
  fprintf(stderr, "EPL poll 0x%2.2x\n", type);
#endif

  ts = temp_string;
  if(epl_job_info->model == MODEL_5700L) 
    {
      /* to be added, just return for now
      ts += sprintf(ts, "%c%c",
        0x??,
	0x00
	);
       */
      return 0;
    }
  else if(epl_job_info->model == MODEL_5800L
          || epl_job_info->model == MODEL_5900L) 
    {
      ts += epl_sprintf_wrap(ts, 2);
      ts += sprintf(ts, "%c%c",
	0x10 + type,
	0x00
	);
    }
  else if(epl_job_info->model == MODEL_6100L) 
    {
      /* unknown */
      return 0;
    }
  
#ifdef EPL_DEBUG
  fprintf(stderr, "Writing %s poll (%i bytes)\n",
          printername[epl_job_info->model], ts - temp_string);
#endif

  e = epl_write_bid(epl_job_info, temp_string, ts - temp_string);
  if(e != ts - temp_string) return -1;

  return 0;
} 
