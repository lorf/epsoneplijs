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

/* we need more than -ansi to use the nanosleep routine */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <stdio.h>
#include <sys/time.h>

#include "epl_time.h"


double get_time_now(void)
{
  struct timeval time_now;
  gettimeofday(&time_now, NULL);
  return time_now.tv_sec + 1e-6 * time_now.tv_usec;
}

void sleep_seconds(double secs)
{
  struct timespec ts;

  if (secs < 0)
    {
      fprintf(stderr, "asked to sleep for %f seconds, IGNORED\n", secs);
    }
  else
    {
    fprintf(stderr, "sleeping for %f seconds\n", secs);
    ts.tv_sec = (int)secs;
    ts.tv_nsec = (int)((secs - (double)ts.tv_sec) * 1000 * 1000 * 1000);
    nanosleep(&ts, NULL);
    }
}

