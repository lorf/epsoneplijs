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

int epl_job_header(EPL_job_info *epl_job_info) 
{
  char temp_string[256];
  char *ts;
  char ritech;
  char tonersave;
  char density;
  char dpi_code1, dpi_code2;
  int e;

#ifdef EPL_DEBUG
  fprintf(stderr, "EPL job header\n");
#endif

  /* Prepare parameters */

  ritech = epl_job_info->ritech;
  tonersave = epl_job_info->toner_save;
  density = epl_job_info->density;
  if(epl_job_info->dpi_h == 300 && epl_job_info->dpi_v == 300)
    {
      dpi_code1 = 0; dpi_code2 = 0;
    }
  else if(epl_job_info->dpi_h == 600 && epl_job_info->dpi_v == 300)
    {
      dpi_code1 = 0; dpi_code2 = 1;
    }
  else if(epl_job_info->dpi_h == 600 && epl_job_info->dpi_v == 600)
    {
      dpi_code1 = 1; dpi_code2 = 0;
    }
  else if(epl_job_info->dpi_h == 1200 && epl_job_info->dpi_v == 600)
    {
      dpi_code1 = 1; dpi_code2 = 1;
    }
  else
    {
      return -1;
    }

  /* Create the string */

  ts = temp_string;
  if(epl_job_info->model == MODEL_5700L)
    {
      ts += sprintf(ts, "%c%c%c%c%c%c%c%c",
	0x00, 0x00,
	dpi_code1, dpi_code2,
	ritech,
	tonersave,
	0x00,
	density
	);
    }
  else if(epl_job_info->model == MODEL_5800L
          || epl_job_info->model == MODEL_5900L)
    {
      ts += sprintf(ts, "\x01b\x001");
      ts += sprintf(ts, "@EJL \x00a");
      ts += sprintf(ts, "@EJL STARTJOB");
      if(epl_job_info->model == MODEL_5900L) 
        {
          ts += sprintf(ts, " MACHINE=\"experiment\" USER=\"epl5x00l_driver_"
	    EPL_VERSION "\"");
        }
      ts += sprintf(ts, "\x00a");
      ts += sprintf(ts, "@EJL EN LA=ESC/PAGE\x00a");
      ts += sprintf(ts, "\x01d");
      ts += sprintf(ts, "4eps{I");
      ts += sprintf(ts, "%c%c%c%c", 0x00, 0x00, 0x00, 0x00);
      ts += sprintf(ts, "\x01d");
      ts += sprintf(ts, "9eps{I");
      ts += sprintf(ts, "%c%c%c%c%c%c%c%c%c",
        0x02, 0x00,
        dpi_code1, dpi_code2,
        ritech,
        tonersave,
        0x00, /* Paper type? */
        density,
        0x00  
        );
    }

#ifdef EPL_DEBUG
  fprintf(stderr, "Writing %s job header (%i bytes)\n",
          printername[epl_job_info->model], ts - temp_string);
#endif
  e = fwrite(temp_string, 1, ts - temp_string, epl_job_info->outfile);
  if(e != ts - temp_string) return -1;

  return 0;
} 
