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

#ifdef HAVE_KERNEL_DEVICE
#include <unistd.h>
#endif

#include "epl_job.h"
#include "epl_usb.h"
#include "epl_usb_replies.h"

/* sleep routine only used here */
void do_subsec_sleep(EPL_job_info *epl_job_info);

/* Bi-directional comminucation routine */

int epl_write_bid(EPL_job_info *epl_job_info, char *buffer, int length)
{
  int ret_write;
  int i_len;
  
  /* use a different buffer to read, to be safe; probably not necessary */
  char in_buf[256]; 

  fprintf(stderr, "We said: (len=%i,%4.4x)", length, length);
  for (i_len = 0 ; i_len < length; i_len++)
    {
      if (i_len % 0x10 == 0)
        {
          fprintf(stderr, "\n-> %4.4x: ", i_len);
        }
      if (i_len % 0x08 == 0)
        {
          fprintf(stderr, " ");
        }
      fprintf(stderr,"%2.2x ", (0xff & buffer[i_len])); 
      if (i_len == 0x2f && length > 0x3f)
        {
          fprintf(stderr, " \n...");
	  i_len = length / 0x10 * 0x10 - 1; /* skip to last row */
        }
    }
  fprintf(stderr, "\n");

  ret_write = epl_write_uni(epl_job_info, buffer, length);  
  
  if (ret_write != length) 
    {
      fprintf(stderr,"Write didn't succeed in bid-writing, aborting: %d\n",ret_write);
      exit(1);
    }
  
  if (epl_job_info->connectivity != VIA_PPORT)
    {
      int ret = 0;
      int code;
      int reply_size;

      code = *buffer;
      /* skip encapsulation */
      /* this is a little hackish, but it's ok for now  --  rora */
      if (*buffer == 0x1d)
        {
          char *tmp = buffer;
          while(*tmp != 0x49) {tmp++;fprintf(stderr,"skip-");} /* we could never stop 8-) */
	  tmp++; code = *tmp;
	}
      /* we could get here with code=0x1b (5900L) */
      if (code < 0 || code >= KNOWN_REPLY_CODES) /* avoid out of bounds condition */
        {
          reply_size = -1;
	}
      else
        {
          reply_size = epl_usb_reply_size[epl_job_info->model][code];
        }

      if (reply_size == -1) /* something is wrong */
        {
          fprintf(stderr, "We don't know the reply size for the code %2.2x, but we try to go on (crossed fingers)\n", 
                  (int) *buffer);
          reply_size = 0;
        }

      if (reply_size != 0) /* we have to read something */
        {
#ifdef HAVE_LIBUSB
          if(epl_job_info->connectivity == VIA_LIBUSB) 
	    {
	      ret = usb_bulk_read(epl_job_info->usb_dev_hd,
			          epl_job_info->usb_in_ep,
			          in_buf,
			          reply_size,
			          EPL_USB_READ_TIMEOUT);
	    } 
#endif

#ifdef HAVE_KERNEL_DEVICE
          if(epl_job_info->connectivity == VIA_KERNEL_USB) 
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

      if(ret > 0)
	{
	  int i_ret;
	  fprintf(stderr, "Printer replied: ");
	  for (i_ret = 0 ; i_ret < ret; i_ret++)
	    {
	      if (i_ret % 16 == 0)
	        {
	          fprintf(stderr, "\n<- %4.4x: ", i_ret);
                }
	      if (i_ret % 8 == 0)
	        {
	          fprintf(stderr, " ");
                }
	      fprintf(stderr,"%2.2x ", (0xff & in_buf[i_ret])); 
	    }
	  fprintf(stderr, "\n");
	}

      if (ret != reply_size) 
	{
	  fprintf(stderr, "expected %d, got %d\n", 
		  reply_size,
		  ret);
	  fprintf(stderr, "**USB reply from printer different size from expected: \n**If you don't see this message too often, it is probaly alright\n**See FAQ.\n");
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
    + time_now.tv_usec - last_wrote_usec ; 

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
