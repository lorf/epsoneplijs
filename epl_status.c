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

#include <string.h>
#include <stdlib.h>

#if defined(HAVE_KERNEL_USB_DEVICE) || defined(HAVE_KERNEL_1284)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif

#ifdef HAVE_KERNEL_1284
#include <sys/ioctl.h>
#include <linux/lp.h>
#endif

#include "epl_job.h"
#include "epl_bid.h"

int epl_status(int argc, char **argv) 
{
  EPL_job_info *epl_job_info;
  char temp_string[256];
  char *ts;
  int model;
  int e = 0;  

  if (argc < 3) /* the IJS plugin is being run stand-alone */ 
    {
      fprintf(stderr, "Epson EPL-5x00L IJS plugin version: %s\n", EPL_VERSION);
      fprintf(stderr, "Copyright (c) 2003 Hin-Tak Leung, Roberto Ragusa\n");

      fprintf(stderr, "Built with <generic> ");
#ifdef HAVE_KERNEL_USB_DEVICE
      fprintf(stderr, "<kernel usb device> ");
#endif
#ifdef HAVE_KERNEL_1284
      fprintf(stderr, "<kernel 1284 device> ");
#endif
#ifdef HAVE_LIBUSB
      fprintf(stderr, "<libusb> ");
#endif
#ifdef HAVE_LIBIEEE1284
      fprintf(stderr, "<libieee1284> ");
#endif
      fprintf(stderr, "support\n");
      fprintf(stderr, "Example usage: ijs_server_epsonepl EPL-5700L /dev/usb/lp0\n");      
      fprintf(stderr, "                                   EPL-5800L libusb\n");      
      fprintf(stderr, "                                   EPL-5900L libieee1284\n");      
      fprintf(stderr, "\n");
      exit(0);
    } 

  epl_job_info = (EPL_job_info *)calloc(1, sizeof(EPL_job_info));  

  model = epl_identify(argv[1]);

  if (model > 0) 
    {
      epl_job_info->model=model;
    }
  else
    {
     /* no need to say anything, the identify routine already did */
      exit(1);
    }
  
#ifdef HAVE_KERNEL_USB_DEVICE
  if (strncmp(argv[2], "/dev/usb",8) == 0)   
    {
      fprintf(stderr, "Using kernel usb device\n");
      epl_job_info->connectivity = VIA_KERNEL_USB;
      epl_job_info->kernel_fd = open(argv[2], O_RDWR |O_SYNC);
      if (epl_job_info->kernel_fd < 0) 
        {
          fprintf(stderr, "Can't open device %s, aborting!\n", argv[1]);
          perror("Kernel device");
          exit(1);
        }                                             
    }
#endif

#ifdef HAVE_KERNEL_1284
  if (strncmp(argv[2], "/dev/lp",7) == 0)   
    {
      fprintf(stderr, "Using kernel 1284 device\n");
      epl_job_info->connectivity = VIA_KERNEL_1284;
      /* usb kernel device time-outs, but this one doesn't, so we either 
	 do it non-block, or set a time out 
      */      
      epl_job_info->kernel_fd = open(argv[2], O_RDWR |O_SYNC);
      if (epl_job_info->kernel_fd < 0) 
        {
          fprintf(stderr, "Can't open device %s, aborting!\n", argv[1]);
          perror("Kernel device");
          exit(1);
        } 
      else
	{
	  struct timeval timeout;
	  timeout.tv_sec = 1;
	  timeout.tv_usec = 0;
	  /* 
	     The printer seem to time out after 100s or 10s regardless of this.
	     Nevermind.
	   */
	  if (ioctl(epl_job_info->kernel_fd,  LPSETTIMEOUT, &timeout) < 0) {
	    perror("ioctl:");
	  }
	}
    }
#endif
  
#ifdef HAVE_LIBUSB
  if (strcmp(argv[2], "libusb") == 0)
    {
      fprintf(stderr, "Using libusb\n");
      epl_job_info->connectivity = VIA_LIBUSB;
    }
#endif

#ifdef HAVE_LIBIEEE1284
  if (strcmp(argv[2], "libieee1284") == 0)
    {
      fprintf(stderr, "Using libieee1284\n");
      epl_job_info->connectivity = VIA_LIBIEEE1284;
    }
#endif

  /* should not have to do this - except the sleep routine needs dpi_v... */
  epl_job_info->dpi_v = 600;

/* FIXME: FLOW_CONTROL? or connectivity!=VIA_STDOUT_PIPE? */
#ifdef USE_FLOW_CONTROL
  epl_bid_init(epl_job_info);
#endif

  ts = temp_string;
  if(epl_job_info->model == MODEL_5700L)
    {
      ts += sprintf(ts, "%c%c",0x07,0x00);
    }
  else if(epl_job_info->model == MODEL_5800L
          || epl_job_info->model == MODEL_5900L)
    {
      ts += epl_sprintf_wrap(ts, 2);
      ts += sprintf(ts, "%c%c",0x11,0x00);
    }
  else if((epl_job_info->model == MODEL_6100L) || (epl_job_info->model == MODEL_6200L))
    {
      ts += epl_sprintf_wrap(ts, 2);
      ts += sprintf(ts, "P%c",0x00);
    }

  /* if there is more parameter at the end, we override the polling defaults */
  if (argc > 3)  
    {
      int value = 0;
      value = atoi(argv[3]);
      *(ts - 2) = value & 0xff;
     }

#ifdef USE_FLOW_CONTROL
  e = epl_write_bid(epl_job_info, temp_string, ts - temp_string);
#endif

  if (e < 0) 
    {
      perror("status:");
    }

  epl_bid_end(epl_job_info);
  exit(0);
}
