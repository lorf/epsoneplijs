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

#ifdef HAVE_KERNEL_USB_DEVICE
#include <unistd.h>
#endif

#include "epl_job.h"
#include "epl_bid.h"

/* sleep routine only used here */
void do_subsec_sleep(EPL_job_info *epl_job_info);

/* generic hex dump routine, prints to stderr */
void hex_dump(char *title, char *prefix, char *buffer, int len, int maxrows);

/* this interprets the replies from the printer */
void epl_interpret_reply(EPL_job_info *epl_job_info, char *buffer, int len, unsigned char code);

/* Bi-directional communication routine */

int epl_write_bid(EPL_job_info *epl_job_info, char *buffer, int length)
{
  int ret_write;
  
  /* use a different buffer to read, to be safe; probably not necessary */
  char in_buf[256]; 

#ifdef EPL_DEBUG
  hex_dump("We said", "->", buffer, length, 5);
#endif

  ret_write = epl_write_uni(epl_job_info, buffer, length);  
  
  if (ret_write != length) 
    {
      fprintf(stderr,"Write didn't succeed in bid-writing, aborting: %d\n",ret_write);
      exit(1);
    }
  
  if (epl_job_info->connectivity != VIA_STDOUT_PIPE)
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
      /* the called function returns -1 for invalid values */
      reply_size = epl_bid_reply_len(epl_job_info->model, code);

      if (reply_size == -1) /* something is wrong */
        {
          fprintf(stderr, "We don't know the reply size for the code %2.2x, but we try to go on (crossed fingers)\n", 
                  code);
          reply_size = 100; /* if we don't know, we try to read a lot */
        }

      if (reply_size != 0) /* we have to read something */
        {
          switch(epl_job_info->connectivity) 
	    {
#ifdef HAVE_LIBUSB
	    case VIA_LIBUSB: 
	      ret = usb_bulk_read(epl_job_info->usb_dev_hd,
			          epl_job_info->usb_in_ep,
			          in_buf,
			          reply_size,
			          EPL_USB_READ_TIMEOUT);
	      break;
#endif
#ifdef HAVE_KERNEL_USB_DEVICE
	    case VIA_KERNEL_USB: 
	      ret = read(epl_job_info->kernel_fd,
		         in_buf,
		         reply_size);
	      break;
#endif
#ifdef HAVE_KERNEL_1284
	    case VIA_KERNEL_1284: 
	      ret = read(epl_job_info->kernel_fd,
		         in_buf,
		         reply_size);
	      break;
#endif
#ifdef HAVE_LIBIEEE1284
	    case VIA_LIBIEEE1284:
	      ret = epl_libieee1284_read(epl_job_info->port, in_buf, reply_size);
	      break;
#endif
	    default:
	      ret = 0;
	    }
	}

      if (ret > 0)
        {
	  hex_dump("Printer replied", "<-", in_buf, ret, 5);
	}

      if (ret != reply_size) 
	{
	  fprintf(stderr, "Reply: expected %d bytes, got %d bytes\n", 
		  reply_size,
		  ret);
	  fprintf(stderr, "**USB reply from printer different size from expected: \n**If you don't see this message too often, it is probably OK\n**See FAQ.\n");

	}

      if (ret > 0 && ret == reply_size)
	{
          epl_interpret_reply(epl_job_info, in_buf, ret, code);
	}
    } 
  return ret_write;
}

/* Unidirectional communication routine */

int epl_write_uni(EPL_job_info *epl_job_info, char *buffer, int length)
{  
  int ret = 0;

  if (epl_job_info->connectivity != VIA_STDOUT_PIPE)
    {
      /* 5700L needs a slow data transfer */
      if (epl_job_info->model == MODEL_5700L)
        {
          do_subsec_sleep(epl_job_info);
        }
      
      switch (epl_job_info->connectivity)
	{
#ifdef HAVE_LIBUSB
	case VIA_LIBUSB:
	  ret = usb_bulk_write (epl_job_info->usb_dev_hd, 
				epl_job_info->usb_out_ep,
				buffer, length, 
				EPL_USB_WRITE_TIMEOUT);
	  break;
#endif
#ifdef HAVE_KERNEL_USB_DEVICE
	case  VIA_KERNEL_USB:
	  ret = write (epl_job_info->kernel_fd, buffer, length);
	  break;
#endif
#ifdef HAVE_KERNEL_1284
	case  VIA_KERNEL_1284:
	  ret = write (epl_job_info->kernel_fd, buffer, length);
	  break;
#endif
	  
#ifdef HAVE_LIBIEEE1284
	case VIA_LIBIEEE1284:
	  ret = epl_libieee1284_write(epl_job_info->port, buffer, length);
	  break;
#endif
	default:
	  ret = 0;
	}
    }
  else  
    {
      ret = fwrite(buffer, 1, length, epl_job_info->outfile);
    }
  return ret;
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
#ifdef USE_FLOW_CONTROL
    case MODEL_5700L:
      epl_job_info->estimated_free_mem = 2*1048576; /* fake */
      epl_57interpret(p, len);
      break;

    case MODEL_5800L:
      /* 5800L uses the routine same as the 5900L */
      epl_59interpret(epl_job_info, p, len, code);
      break;

    case MODEL_5900L:
      epl_59interpret(epl_job_info, p, len, code);
      break;

    case MODEL_6100L:
      epl_job_info->estimated_free_mem = 2*1048576; /* fake */
      break;
#endif
    default:
      break;
    }
}

