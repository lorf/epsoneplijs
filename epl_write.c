/*
  Copyright (c) 2003 Hin-Tak Leung
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

/* we need more than -ansi to use the nanosleep routine */
#define _POSIX_C_SOURCE 199309L
#include <time.h>

#include <stdlib.h>

#ifdef HAVE_KERNEL_DEVICE
#include <unistd.h>
#endif

#include "epl_job.h"
#include "epl_usb.h"
#include "epl_usb_replies.h"

/* sleep routine only used here */
void do_subsec_sleep(EPL_job_info *epl_job_info);

/* generic hex dump routine, prints to stderr */
void hex_dump(char *title, char *prefix, char *buffer, int len, int maxrows);

/* this interprets the replies from the printer */
void epl_interpret_reply(EPL_job_info *epl_job_info, char *buffer, int len, unsigned char code);

/* sleep */
void sleep_milliseconds(int ms);


/* Bi-directional communication routine */

int epl_write_bid(EPL_job_info *epl_job_info, char *buffer, int length)
{
  int ret_write;
  
  /* use a different buffer to read, to be safe; probably not necessary */
  char in_buf[256]; 

  hex_dump("We said", "->", buffer, length, 5);

  ret_write = epl_write_uni(epl_job_info, buffer, length);  
  
  if (ret_write != length) 
    {
      fprintf(stderr,"Write didn't succeed in bid-writing, aborting: %d\n",ret_write);
      exit(1);
    }
  
  if (epl_job_info->connectivity != VIA_PPORT)
    {
      int ret = 0;
      unsigned char code;
      int reply_size;

      code = *buffer;
      /* skip encapsulation */
      /* this is a little hackish, but it's ok for now  --  rora */
      if (*buffer == 0x1d)
        {
          int i;
          for (i = 0 ;  i < length - 1 ; i++)
            {
              fprintf(stderr,"skip-");
	      if (buffer[i] == 0x49)
                {
	          code = buffer[i+1];
                  break;
	        }
	    }
	}
      /* we could get here with code=0x1b (5900L) or other values (6100L) */
      if (code < KNOWN_REPLY_CODES) /* avoid out of bounds condition */
        {
          reply_size = epl_usb_reply_size[epl_job_info->model][code];
	}
      else
        {
          reply_size = -1;
        }

      if (reply_size == -1) /* something is wrong */
        {
          fprintf(stderr, "We don't know the reply size for the code %2.2x, but we try to go on (crossed fingers)\n", 
                  code);
          reply_size = 0;
        }

      if (reply_size != 0) /* we have to read something */
        {
#ifdef HAVE_LIBUSB
          if (epl_job_info->connectivity == VIA_LIBUSB) 
	    {
	      ret = usb_bulk_read(epl_job_info->usb_dev_hd,
			          epl_job_info->usb_in_ep,
			          in_buf,
			          reply_size,
			          EPL_USB_READ_TIMEOUT);
	    } 
#endif

#ifdef HAVE_KERNEL_DEVICE
          if (epl_job_info->connectivity == VIA_KERNEL_USB) 
	    {
	      ret = read(epl_job_info->kernel_fd,
		         in_buf,
		         reply_size);
	    }
#endif
        }
      else
        {
          ret = 0;
	}

      if (ret != reply_size) 
	{
	  fprintf(stderr, "Reply: expected %d bytes, got %d bytes\n", 
		  reply_size,
		  ret);
	  fprintf(stderr, "**USB reply from printer different size from expected: \n**If you don't see this message too often, it is probably OK\n**See FAQ.\n");
	}

      if (ret > 0)
	{
          hex_dump("Printer replied", "<-", in_buf, ret, 5);
          epl_interpret_reply(epl_job_info, in_buf, ret, code);
	}

    } 
  return ret_write;
}

/* Unidirectional communication routine */

int epl_write_uni(EPL_job_info *epl_job_info, char *buffer, int length)
{  
  int ret = 0;

  if (epl_job_info->connectivity != VIA_PPORT)
    {
      /* 5700L needs a slow data transfer */
      if (epl_job_info->model == MODEL_5700L)
        {
          do_subsec_sleep(epl_job_info);
        }

#ifdef HAVE_LIBUSB
      if(epl_job_info->connectivity == VIA_LIBUSB) 
	{
	  ret = usb_bulk_write (epl_job_info->usb_dev_hd, 
				epl_job_info->usb_out_ep,
				buffer, length, 
				EPL_USB_WRITE_TIMEOUT);
	}
#endif
      
#ifdef HAVE_KERNEL_DEVICE
      if(epl_job_info->connectivity == VIA_KERNEL_USB) 
	{
	  ret = write (epl_job_info->kernel_fd, buffer, length);
	}
#endif
            
     return ret;
    }
  else  
    {
      return fwrite(buffer, 1, length, epl_job_info->outfile);
    }
}

/* 
   Sleeping routine to do flow control. This code assumes that the execution time
   of any part of itself is negligible (i.e. < 1 micro-seconds), and we never
   need to sleep for more than 1 second.
   ***So there is a fair amount of cutting corners here and there...careful***

   On very slow or very busy machines, this assumption may not be true.
*/
/* 
   Flow control parameter:
   According to the spec, the 5700L can do 8 pages per minutes; i.e. 
   7 seconds per page; the max page length is about 14in, containing about 
   140 stripes at max resolution. So the printer must be able to process
   a stripe within 0.05 seconds, or 50,000 micro-seconds. Hence this 
   number. One may need to increase or look for problems elsewhere
   if a value of 50,000 doesn't work reliably.

   The smallest value chosen so there is no unnecessary waits.
   The wait route will use this value for high resolution, and
   double it when the vertical resolution is only 300dpi. 
   (only 300 or 600 are valid values)  
*/
#define USEC_BETWEEN_WRITES_5700L 50000

void do_subsec_sleep(EPL_job_info *epl_job_info) 
{
#ifdef USE_FLOW_CONTROL
  long last_wrote_sec, last_wrote_usec;
  unsigned long usec_interval;
  struct timeval time_now;

  long sleep_time_between_writes;

  if( epl_job_info->dpi_v <= 600) 
    { 
      sleep_time_between_writes = ( 600 / epl_job_info->dpi_v ) * USEC_BETWEEN_WRITES_5700L ;
    } 
  else
    {
      /* just in case a future printer might have dpi_v > 600 */
      sleep_time_between_writes =  USEC_BETWEEN_WRITES_5700L;
    }

  last_wrote_sec = epl_job_info->time_last_write.tv_sec;
  last_wrote_usec = epl_job_info->time_last_write.tv_usec;
  gettimeofday(&time_now,NULL);

  usec_interval = (time_now.tv_sec - last_wrote_sec) * 1000000 
    + time_now.tv_usec - last_wrote_usec; 

  if (usec_interval < sleep_time_between_writes)
    {
      /* 
	 probably should also check usec_interval > 0, but it is never negative... 
         like-wise, hopefully we should never need to sleep for more than one seconds 
	 between stripes
      */
      struct timespec time_to_sleep;
      time_to_sleep.tv_sec = 0 ;
      time_to_sleep.tv_nsec = (sleep_time_between_writes - usec_interval) * 1000;
      nanosleep(&time_to_sleep, NULL);      
    }
  /* store the time away so we can read it next time this routine is called */
  gettimeofday(&(epl_job_info->time_last_write),NULL);
#endif
}

/*
   This routine dump hexadecimal data in a easily readable format.
   Useful for debugging.
*/
void hex_dump(char *title, char *prefix, char *buffer, int len, int maxrows)
{
  int i;
  fprintf(stderr, "%s: (len=%i=0x%4.4x)", title, len, len);
  for (i = 0 ; i < len ; i++)
    {
      if (i == 0x10 * (maxrows - 2) && len > 0x10 * maxrows)
        {
          fprintf(stderr, " \n...");
	  i = len / 0x10 * 0x10; /* skip to last row */
        }
      if (i % 0x10 == 0)
        {
          fprintf(stderr, "\n%s %4.4x: ", prefix, i);
               }
      if (i % 0x08 == 0)
        {
          fprintf(stderr, " ");
        }
      fprintf(stderr,"%2.2x ", (0xff & buffer[i])); 
    }
  fprintf(stderr, "\n");
}

/*
   This routine interprets the reply.
   We can see several parameters and printer condition.
   Some of them tell us to slow down data transfer to avoid printer
   congestion.
*/
void epl_interpret_reply(EPL_job_info *epl_job_info, char *buffer, int len, unsigned char code)
{
  unsigned char *p;
  
  p = (unsigned char *)buffer;

  switch (epl_job_info->model)
    {
    case MODEL_5700L:
      epl_job_info->estimated_free_mem = 2*1048576; /* fake */
      break;

    case MODEL_5800L:
    case MODEL_5900L:
      {
        int free_memory;
        static int last_waited_milliseconds = 1;
	
        if (len < 18)
          {
            /* This condition should never happen */
            fprintf(stderr, "**Reply size too short - This condition should never happen.");
          }
        if (len != 18 + p[0x11])
          {
            /* This condition should never happen */
            fprintf(stderr, "**Reply size inconsistent - This condition should never happen.");
          }

        fprintf(stderr, "Printer tells:\n");
        free_memory = (p[0x04] << 16) | (p[0x05] << 8) | p[0x06];
        fprintf(stderr, "  free memory      = 0x%8.8x\n", free_memory);

        fprintf(stderr, "  status(0x07)     = 0x%2.2x:",p[0x07]);
        if (p[0x07] & 0x80) fprintf(stderr, " UNKNOWN_80");
        if (p[0x07] & 0x40) fprintf(stderr, " UNKNOWN_40");
        if (p[0x07] & 0x20) fprintf(stderr, " UNKNOWN_20");
        if (p[0x07] & 0x10) fprintf(stderr, " JOB_PENDING");
        if (p[0x07] & 0x08) fprintf(stderr, " UNKNOWN_08");
        if (p[0x07] & 0x04) fprintf(stderr, " ENERGY_SAVING");
        if (p[0x07] & 0x02) fprintf(stderr, " WARMING_UP");
        if (p[0x07] & 0x01) fprintf(stderr, " JOB_FINISHED?");
        fprintf(stderr, "\n");

        fprintf(stderr, "  status(0x09)     = 0x%2.2x:",p[0x09]);
        if (p[0x09]&0x80) fprintf(stderr, " UNKNOWN_80");
        if (p[0x09]&0x40) fprintf(stderr, " UNKNOWN_40");
        if (p[0x09]&0x20) fprintf(stderr, " UNKNOWN_20");
        if (p[0x09]&0x10) fprintf(stderr, " UNKNOWN_10");
        if (p[0x09]&0x08) fprintf(stderr, " UNKNOWN_08");
        if (p[0x09]&0x04) fprintf(stderr, " OPEN_COVER");
        if (p[0x09]&0x02) fprintf(stderr, " NO_PAPER");
        if (p[0x09]&0x01) fprintf(stderr, " UNKNOWN_01");
        fprintf(stderr, "\n");

        fprintf(stderr, "  pages just print = %i\n", (p[0x0d] << 8) | p[0x0e]);

        if (p[0x11] == 0)
          {
            fprintf(stderr, "no extension\n");
          }
        else if (p[0x11] == 0x01)
          {
            fprintf(stderr, "0x01 extension:\n");
          }
        else if (p[0x11] == 0x0f)
          {
            fprintf(stderr, "0x0f extension:\n");
            fprintf(stderr, "  Installed memory = %iMiB\n", p[0x1b]);
          }
        else if (p[0x11] == 0x11)
          {
            fprintf(stderr, "0x11 extension:\n");
            fprintf(stderr, "  printed pages    = %i\n", (p[0x19] << 8) | p[0x1a]);
            fprintf(stderr, "  toner supply     = %i%%\n", p[0x1b]);
            fprintf(stderr, "  imaging supply   = %i%%\n", p[0x1c]);
            fprintf(stderr, "  paper supply     = %i%%\n", p[0x1d]);
          }
        else
          {
            fprintf(stderr, "unknown 0x%2.2x extension, ignoring\n", p[0x11]);
          }

        fprintf(stderr, "updating freemem estimate from 0x%8.8x",
	        epl_job_info->estimated_free_mem);
        epl_job_info->estimated_free_mem = free_memory;
        fprintf(stderr, " to 0x%8.8x\n", epl_job_info->estimated_free_mem);

        /* If the buffer is almost full, go to sleep.
           Increase sleeping time if this condition persists.
           So, calling this function continuosly is not a problem.
             --  rora
         */
        if (free_memory < FREE_MEM_LOW_LEVEL)
          {
            int ms;
            ms = last_waited_milliseconds * 2; /* exponential back off */
            if (ms > 5000 ) ms = 5000; /* no more than five second */
            sleep_milliseconds(ms);
            last_waited_milliseconds = ms;
          }
        else
          {
            last_waited_milliseconds = 1;
          }
      }
      break;

    case MODEL_6100L:
      epl_job_info->estimated_free_mem = 2*1048576; /* fake */
      break;
    }
}

/*
   sleep for (at least) some milliseconds
*/
void sleep_milliseconds(int ms)
{
  struct timespec ts;

  fprintf(stderr, "sleeping %i milliseconds\n", ms);
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = 1000000 * (ms % 1000);
  nanosleep(&ts, NULL);
}
