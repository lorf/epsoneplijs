/*
  Copyright (c) 2003 Hin-Tak Leung
*/  

/*
   skeleton transport code for debugging bid problems
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

/* strdup need this */
#ifndef _SVID_SOURCE
#define _SVID_SOURCE 1 
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>

#include "epl_job.h"
#include "epl_bid.h"

int epl_null_write(char *ts, int length)
{
  return length;
}

int epl_null_read(char *inbuf, int length)
{	
  return length;
}

void epl_null_init(EPL_job_info *epl_job_info)
{
  return;
}


void epl_null_end(EPL_job_info *epl_job_info)
{
  return;
}
