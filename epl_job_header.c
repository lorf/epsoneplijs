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
#include <string.h>
#include "epl_job.h"

int epl_job_header(EPL_job_info *epl_job_info)
{
  char temp_string[256];
  char *ts;
  int ts_count; /* how many strings */
  char *ts_start_idx[5]; /* where strings begin */
  char ritech;
  char tonersave;
  char density;
  char dpi_code1, dpi_code2;
  char papertype;
  int i, e;
  char data_block[7];  /* +1 for string termination */
  char data_block_6xL[9];  /* +1 for string termination */

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
  papertype = 0 ;  /* normal paper; other options are transparency, etc */

  /* 
     Much of the 6100L options are less inflexible - emit warning, and possibly
     change to abort if the hardware really does not support it.
  */  

  if (epl_job_info->model == MODEL_6100L)
    {
      if(ritech != 0x00) 
	{
	  fprintf(stderr, "6100L: Hardware may not support RITech. Use at your own risk!\n");
	}

      if(tonersave != 0x00) 
	{
	  fprintf(stderr, "6100L: Hardware-based Toner Save may not be supported. Use this option at your own risk!\n");
	}

      if(epl_job_info->dpi_h != 600 || epl_job_info->dpi_v != 600) 
	{
	  fprintf(stderr, "6100L: Hardware may not support resolution %d x %d - Use this resolution at your own risk!\n", 
		  epl_job_info->dpi_h, epl_job_info->dpi_v);
	}
    }

  /* model-independent part */
  
  sprintf(data_block, "%c%c%c%c%c%c",
	dpi_code1, dpi_code2,
	ritech,
	tonersave,
	papertype, 
	density
	);
  /* 
     6100L notes:
     ===========
     papertype                   \163
     density                     \164
     ignore selected paper size  \167
     auto continue              \168
  */
  
  sprintf(data_block_6xL, "%c%c%c%c%c%c%c%c",
	  0x00, 0x00,
	  0x00, /* Ignore selected paper size = False */
	  0x00, /* Auto continue = False */
	  0x00, 0x00, 0x00, 0x00
	  );


  /* model-specific part: Create the string */

  ts = temp_string;
  ts_count = 0;
  ts_start_idx[ts_count++] = ts;
  if (epl_job_info->model == MODEL_5700L)
    {
      ts += sprintf(ts, "%c%c",0x00, 0x00);
      memcpy(ts, data_block, 6); ts += 6;
      ts_start_idx[ts_count++] = ts;
    }
  else if (epl_job_info->model == MODEL_5800L
	   || epl_job_info->model == MODEL_5900L
	   || epl_job_info->model == MODEL_6100L)
    {
      ts += sprintf(ts, "\x01b\x001");
      ts += sprintf(ts, "@EJL \x00a");
      ts += sprintf(ts, "@EJL STARTJOB");

      if (epl_job_info->model == MODEL_5900L)  
        {
          ts += sprintf(ts, " MACHINE=\"experiment\" USER=\"epl5x00l_driver_"
	    EPL_VERSION "\"");
        }

      if (epl_job_info->model == MODEL_6100L)  
	{
	  /* probably the same as 5900L, just being conservative */
	  ts += sprintf(ts, " MACHINE=\"test_box \" USER=\"EG\"");
	}

      ts += sprintf(ts, "\x00a");
      ts += sprintf(ts, "@EJL EN LA=ESC/PAGE\x00a");
      ts_start_idx[ts_count++] = ts;

      if (epl_job_info->model == MODEL_5800L
	  || epl_job_info->model == MODEL_5900L)
	{
	  ts += epl_sprintf_wrap(ts,4);
	  ts += sprintf(ts, "%c%c%c%c", 0x00, 0x00, 0x00, 0x00);
          ts_start_idx[ts_count++] = ts;
	  ts += epl_sprintf_wrap(ts,9);
	  ts += sprintf(ts, "%c%c", 0x02, 0x00);
	  memcpy(ts, data_block, 6); ts += 6;
	  ts += sprintf(ts, "%c", 0x00);
          ts_start_idx[ts_count++] = ts;
	}
      else if (epl_job_info->model == MODEL_6100L)
	{
	  ts += epl_sprintf_wrap(ts,68);
	  ts += sprintf(ts, "@%c%c%ctest_box  ", 0x00, 0x00, 0x00);
	  ts += sprintf(ts, "                  ");      /* Don't touch these! */
	  ts += sprintf(ts, "                  ");      /* The padding is significant! */
	  ts += sprintf(ts, "                  ");
	  ts += epl_sprintf_wrap(ts, 16);
	  ts += sprintf(ts, "B%c", 0x00);
	  memcpy(ts, data_block, 6); ts += 6;
	  memcpy(ts, data_block_6xL, 8); ts += 8;
          ts_start_idx[ts_count++] = ts;
	}      
    }

#ifdef EPL_DEBUG
  fprintf(stderr, "Writing %s job header (%i bytes in %i parts)\n",
          printername[epl_job_info->model], ts - temp_string, ts_count);
#endif

  for (i = 0 ; i < ts_count - 1 ; i++)
    {
#ifdef EPL_DEBUG
      fprintf(stderr,"string %i from %p to %p\n",
              i,
	      ts_start_idx[i],
	      ts_start_idx[i+1]);
#endif
      e = epl_write_bid(epl_job_info,
                        ts_start_idx[i],
			ts_start_idx[i+1] - ts_start_idx[i]);
      if(e != ts_start_idx[i+1] - ts_start_idx[i]) return -1;
    }
  return 0;
} 
