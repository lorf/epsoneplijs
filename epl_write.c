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

#include <stdlib.h>
#include <string.h>

/* read() and write() */
#if defined(HAVE_KERNEL_USB_DEVICE) || defined(HAVE_KERNEL_1284) 
#include <unistd.h>
#endif

#include "epl_job.h"
#include "epl_bid.h"
#include "epl_time.h"

/* generic hex dump routine, prints to stderr */
static void hex_dump(char *title, char *prefix, char *buffer, int len, int maxrows);

/* this interprets the replies from the printer */
void epl_interpret_reply(EPL_job_info *epl_job_info, char *buffer, int len, unsigned char code);

/* slow down the 5700l */
void do_5700l_slowdown(EPL_job_info *epl_job_info);

/* slow down if printer memory almost full */
void do_free_mem_slowdown(EPL_job_info *epl_job_info);


/* Bi-directional communication routine */

int epl_write_bid(EPL_job_info *epl_job_info, char *buffer, int length)
{
  int ret_write;
  
  /* use a different buffer to read, to be safe; probably not necessary */
  char in_buf[MAX_READ_SIZE]; 
  memset(in_buf, 0x00, MAX_READ_SIZE);
#ifdef EPL_DEBUG
  hex_dump("We said", "->", buffer, length, 5);
#endif

  ret_write = epl_write_uni(epl_job_info, buffer, length);  
  
  if (ret_write != length) 
    {
      fprintf(stderr,"Write didn't succeed in bid-writing, aborting: %d\n", ret_write);
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
      /* 5700L doesn't need skipping, and it may interfer with probing -- HTL */
      if ((*buffer == 0x1d) && (epl_job_info->model != MODEL_5700L))
        {
          int i;
          for (i = 0; i < length - 1; i++)
            {
#ifdef EPL_DEBUG
              fprintf(stderr, "skip-");
#endif
	      if (buffer[i] == 0x49)
                {
	          code = buffer[i+1];
                  break;
	        }
	    }
#ifdef EPL_DEBUG
	  fprintf(stderr, "\n");
#endif
	}
      /* we could get here with code=0x1b (5900L) or other values (6100L/6200L) */
      /* the called function returns -1 for invalid values */
      reply_size = epl_bid_reply_len(epl_job_info->model, code);

      if (reply_size == -1) /* something is wrong */
        {
          fprintf(stderr, "We don't know the reply size for the code %2.2x, but we try to go on "
	          "and read a lot (crossed fingers)\n", 
                  code);
          reply_size = MAX_READ_SIZE; /* if we don't know, we try to read a lot */
        }

      if (reply_size != 0) /* we have to read something */
        {
	  ret = epl_read_uni(epl_job_info, in_buf, reply_size);
	}

      if ((ret >= 0) && (ret == reply_size))
	{
	  /* size is as expected */
		if (ret > 0)
	    {
			if (code != in_buf[0])
			{
				/* printer's respond did not start with the command code we send, something is wrong */
				fprintf(stderr, "Driver's interpretation of the reply from the printer is suspect.\n");
				fprintf(stderr, "Please send the hex dump below to the developers:\n");
				hex_dump("Printer replied", "<-", in_buf, ret, 5);				
			}
			else
			{
				/* everything is as expected */
				/* interprete it */
				epl_interpret_reply(epl_job_info, in_buf, ret, code);
            }
		}
	}
      else 
	{
	  if (ret > 0) 
	    {
	      /* unexpected size, but ret > 0 , we should hex dump */
	      fprintf(stderr, "Reply: expected %d bytes, got %d bytes\n", 
		      reply_size,
		      ret);
	      fprintf(stderr, "**Bidirectional reply from printer different size from expected: \n**If you don't see this message too often, it is probably OK\n**See FAQ.\n");
	      
	      hex_dump("Printer replied", "<-", in_buf, ret, 5);
	    }
	  else
	    {
	      /* if we got here, it means read didn't succeed and ret <= 0 */
	      fprintf(stderr, "Reply: expected %d bytes, unsuccessful read status: %d\n", 
		      reply_size,
		      ret);
	      /* 
		 When this happens, it might be useful to know what recently happened at the OS level,
		 although this is likely to be confusing...
	      */
	      perror("Bid read:");
	    }
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
      switch (epl_job_info->connectivity)
	{
	case VIA_NOWHERE:
		ret = epl_null_write(buffer, length);
		break;
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
	  break;
	}
    }
  else  
    {
      ret = fwrite(buffer, 1, length, epl_job_info->outfile);
    }
  return ret;
}

int epl_read_uni(EPL_job_info *epl_job_info, char *in_buf, int reply_size)
{
  int ret = 0;
  /* reset last free mem, in case something go wrong with the read and it doesn't get updated */
  epl_job_info->free_mem_last_update = TOTAL_MEM_DEFAULT_VALUE;
  switch(epl_job_info->connectivity) 
    {
    case VIA_NOWHERE:
      ret = epl_null_read(in_buf, reply_size);
      break;
      
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
      break;
    }
  epl_job_info->bytes_sent_after_last_update = 0;
  epl_job_info->stripes_sent_after_last_update = 0;
  return ret;
}


/*
   This routine dump hexadecimal data in a easily readable format.
   Useful for debugging.
*/
void hex_dump(char *title, char *prefix, char *buffer, int len, int maxrows)
{
  int i;
  fprintf(stderr, "%s: (len=%i=0x%4.4x)", title, len, len);
  for (i = 0; i < len; i++)
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
      epl_57interpret(p, len);
      break;

    case MODEL_5800L:
      /* 5800L uses the routine same as the 5900L */
      epl_59interpret(epl_job_info, p, len);
      break;

    case MODEL_5900L:
      epl_59interpret(epl_job_info, p, len);
      break;

    case MODEL_6100L:
      /* 6100L uses the routine same as the 6200L */
      epl_62interpret(epl_job_info, p, len);
      break;

    case MODEL_6200L:
      epl_62interpret(epl_job_info, p, len);
      break;
#endif
    default:
      break;
    }
}

/* 
   Sleeping routine to do flow control on 5700L. We sleep between stripes to
   avoid congesting the printer. This code assumes that the execution time
   of any part of itself is negligible (i.e. < 1 micro-seconds)
   ***So there is a fair amount of cutting corners here and there...careful***

   On very slow or very busy machines, this assumption may not be true.
   
   How is flow control parameter calculated:
   According to the spec, the 5700L can do 8 pages per minutes; i.e. 
   7 seconds per page; the max page length is about 14in, containing about 
   140 stripes at max resolution. So the printer must be able to process
   a stripe within 0.050 seconds. Hence this number. One may need to
   increase or look for problems elsewhere if a value of 0.050 doesn't
   work reliably.

   The smallest value chosen so there is no unnecessary waits.
   The wait route will use this value for high resolution, and
   double it when the vertical resolution is only 300dpi. 
   (only 300 or 600 are valid values)  
*/
#define SECS_BETWEEN_WRITES_5700L 0.050

void do_5700l_slowdown(EPL_job_info *epl_job_info) 
{
#ifdef USE_FLOW_CONTROL
  double now;
  double time_between_writes;
  double sec_interval;

  now = get_time_now();
  /* sleep time is vertical resolution dependent */
  time_between_writes = SECS_BETWEEN_WRITES_5700L * 600 / epl_job_info->dpi_v;

  /* should we delay? */
  sec_interval = epl_job_info->time_last_write_stripe - now;
  if (sec_interval < time_between_writes)
    {
      sleep_seconds(time_between_writes - sec_interval);
    }
  /* upper levels will keep time_last_write_stripe updated for us */
#endif
}

/*
   Sleeping routine that avoids filling the memory of the printer.
   We keep track of the free memory and sleep when the value is
   too low.
*/

void do_free_mem_slowdown(EPL_job_info *epl_job_info)
{  
#ifdef USE_FLOW_CONTROL
  static double seconds_to_wait = 0.001;
  EPL_job_info *e = epl_job_info; /* for brevity */ 
  double time_now;
  int estimated_free_mem = 2000000;

  /*
  If our estimated free mem is too low, we check the real
  value and refuse to go ahead if it is actually low.
  We indefinitely wait for some memory to be freed.
  So, yes, we can get stuck here for ever; and this is
  right, because if the printer goes out of paper on
  a 200 page job, the buffer fills up and we loop
  here until (maybe hours later) someone adds more
  paper.
  This also means we've no way of exit if a page needs
  more memory than physically installed in the printer.
  So what? We're hopeless anyway.
  (a good work around for this problem needs more work
  and could be implemented in the future)
  
  We're overestimating the amount of memory we're consuming.
  This is not a problem, it simply triggers the above
  verification earlier.
  A factor of 2 could be enough, according to my logs.
    --  rora
  
  Further analysis of 5900L logs suggest a factor of 1
  and an offset of 64 (the factor is maybe a little
  higher than 1).
  But there is some noise in a short time interval
  (a couple of stripes), maybe related to buffering
  and timing issues, so we stay on the safe side as
  this is not critical.
    --  rora
  
  Last update: we use a factor of 1.125 plus 64 bytes
  per stripe and an extra 80000 byte if we wrote
  a stripe recently.
    --  rora
  */          
  do
    {
      time_now = get_time_now();
#ifdef EPL_DEBUG
      fprintf(stderr, "time_now=%f, time_last_write_stripe=%f\n",time_now,e->time_last_write_stripe);
#endif
      /* est_mem = bytes we sent + 12.5% safety 
	 + extra overhead per stripe for safety 
	 + the printer could be lying if we just sent something, */
      /* So we account for an extra big stripe (80000 > worst case) 
	 if 3 seconds have not passed yet */
      estimated_free_mem = e->free_mem_last_update
	- e->bytes_sent_after_last_update - e->bytes_sent_after_last_update / 8
	- 64 * e->stripes_sent_after_last_update
	- (time_now - e->time_last_write_stripe < 3. ? 80000 : 0);
      if (estimated_free_mem < FREE_MEM_LOW_LEVEL)
        {
          /*
             If the buffer is almost full, go to sleep.
             Increase sleeping time if this condition persists.
             So, looping on this code continuosly is not a problem.
               --  rora
          */
          double s;
          s = seconds_to_wait;
          fprintf(stderr, "low est. free memory 0x%8.8x, going to poll after %f seconds\n",
                  estimated_free_mem, s);
          sleep_seconds(s);
          s = s * 2; /* exponential back off */
          if (s > 5 ) s = 5; /* no more than five second */
	  /*s=0.001;*/ /* hard coded for tests */
          seconds_to_wait = s;
          epl_poll(epl_job_info, 2); /* we just need the memory value */
          continue;
        }
      else
        {
          seconds_to_wait = 0.001;
	  break;
        }
    } while (1);
#endif
}

void epl_permission_to_write_stripe(EPL_job_info *epl_job_info)
{
  if (epl_job_info->connectivity == VIA_STDOUT_PIPE) return;
  
  /* 5700L needs an extra delay to slow down data transfer */
  if (epl_job_info->model == MODEL_5700L)
    {
      do_5700l_slowdown(epl_job_info);
      /* ATTENTION: the following free_mem handling could be
         useful for 5700L, but we're currently keeping it
	 disabled, so we just return.
	   -- rora
      */
      return;
    }

  /* Now we check how much free memory the printer has */
    do_free_mem_slowdown(epl_job_info);
}
