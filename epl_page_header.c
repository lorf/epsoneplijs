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
#include <unistd.h>
#include "epl_job.h"
#include "epl_usb.h"

int epl_page_header(EPL_job_info *epl_job_info) 
{
  char temp_string[256];
  char *ts;
  char paper_code;
  char stripe_size;
  int bytes_per_row_2;
  int hor_pixels;
  int ver_pixels;
  int total_stripes;
  int copies;
  int cust_paper_hor;
  int cust_paper_ver;
  int e;


#ifdef EPL_DEBUG
  fprintf(stderr, "EPL page header\n");
#endif

  /* Prepare parameters */

  paper_code = (char)0xff;	/* 0xff is custom size */
  stripe_size = 64;
  bytes_per_row_2 = epl_job_info->bytes_per_row_2;
  hor_pixels = epl_job_info->pixel_h;
  ver_pixels = epl_job_info->pixel_v;
  total_stripes = epl_job_info->total_stripes;
  copies=1;
  cust_paper_hor = epl_job_info->paper_size_mm_h;
  cust_paper_ver = epl_job_info->paper_size_mm_v;
  

  /* Create the string */

  ts = temp_string;
  if(epl_job_info->model == MODEL_5700L)
    {
      ts += sprintf(ts, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
	0x02, 0x00,
        paper_code,
        stripe_size,
        bytes_per_row_2 >> 8, bytes_per_row_2,
        0x00,
        0x00,
        0x00,
        0x00,
        ver_pixels >> 8, ver_pixels,
        hor_pixels >> 8, hor_pixels,
        total_stripes >> 8, total_stripes,
        0xff, /* tray */
        0x00,
        copies,
        0xff,
        0xfe,
        cust_paper_hor >> 8, cust_paper_hor,
        cust_paper_ver >> 8, cust_paper_ver
        );
    }
  else if(epl_job_info->model == MODEL_5800L
          || epl_job_info->model == MODEL_5900L)
    {
      ts += sprintf(ts, "\x01d");
      ts += sprintf(ts, "26eps{I");
      ts += sprintf(ts, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
        0x04,0x00,
        paper_code,
        stripe_size,
        bytes_per_row_2 >> 8, bytes_per_row_2,
        0x00,
        0x00,
        0x00,
        0x00,
        ver_pixels >> 8, ver_pixels,
        hor_pixels >> 8, hor_pixels,
        total_stripes >> 8, total_stripes,
        0x00, /* tray */
        0x00,
        copies,
        0xff,
        0xfe,
        cust_paper_hor >> 8, cust_paper_hor,
        cust_paper_ver >> 8, cust_paper_ver,
        0x01
	);
    }

#ifdef EPL_DEBUG
  fprintf(stderr, "Writing %s page header (%i bytes)\n",
          printername[epl_job_info->model], ts - temp_string);
#endif

  e = epl_write_bid(epl_job_info, temp_string, ts - temp_string);
  if(e != ts - temp_string) return -1;

  if ((epl_job_info->connectivity != VIA_PPORT)
      && (epl_job_info->model == MODEL_5700L))
    {
      epl_usb_mid(epl_job_info);
    }
  
  return 0;
} 
