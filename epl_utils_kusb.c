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

#include <sys/ioctl.h>

#include <stdlib.h>
#include <string.h>

#include "epl_job.h"
#include "epl_bid.h"

/* Copied somewhat from /usr/src/linux/drivers/usb/printer.c: */     
#define MAX_DEVICE_ID_SIZE     1024
#define LPIOC_GET_DEVICE_ID    _IOC(_IOC_READ ,'P',1,MAX_DEVICE_ID_SIZE)     
#define LPIOC_GET_PROTOCOLS    _IOC(_IOC_READ ,'P',2,sizeof(int[2]))
#define LPIOC_SET_PROTOCOL     _IOC(_IOC_WRITE,'P',3,0)                 
#define LPIOC_HP_SET_CHANNEL   _IOC(_IOC_WRITE,'P',4,0)         
#define LPIOC_GET_BUS_ADDRESS  _IOC(_IOC_READ ,'P',5,sizeof(int[2]))
#define LPIOC_GET_VID_PID      _IOC(_IOC_READ ,'P',6,sizeof(int[2]))

void epl_kernel_init(EPL_job_info *epl_job_info)
{
  /* the kernel device doesn't need init, but we'll do some check anyway */
  int ioctl_args[2];
  int model;
  unsigned char ioctl_string[MAX_DEVICE_ID_SIZE];  /* This 1024 comes from the kernel header */
  unsigned char *ioctl_string_shifted_by_two;
  int length;
 
  ioctl(epl_job_info->kernel_fd, LPIOC_GET_VID_PID, &ioctl_args);
  if ((ioctl_args[0] == USB_VENDOR_EPSON)
      && (ioctl_args[1] == USB_PRODUCT_5700L))
    {
      fprintf (stderr,"Found 5700L\n");
    }
  else if ((ioctl_args[0] == USB_VENDOR_EPSON)
           && (ioctl_args[1] == USB_PRODUCT_5800L)) /* and 5900L, 6100L */
    {
      fprintf (stderr,"Found 5800L/5900L/6100L\n");
    }
  else
    {
      fprintf (stderr,"Unknown device: %04X/%04X, aborting!\n", ioctl_args[0], ioctl_args[1]);
      exit(1);
    }

  ioctl(epl_job_info->kernel_fd, LPIOC_GET_PROTOCOLS, &ioctl_args);
  fprintf (stderr,"IOC Get Protocols: %04X/%04X\n", ioctl_args[0], ioctl_args[1]);
  ioctl(epl_job_info->kernel_fd, LPIOC_GET_BUS_ADDRESS, &ioctl_args);
  fprintf (stderr,"IOC Bus Address: %04X/%04X\n", ioctl_args[0], ioctl_args[1]);

  ioctl(epl_job_info->kernel_fd, LPIOC_GET_DEVICE_ID, ioctl_string);
  length = (ioctl_string[0] << 8) + ioctl_string[1] - 2;
  ioctl_string_shifted_by_two = ioctl_string + 2; /* to the actual string */
  ioctl_string_shifted_by_two[length] = 0; /* string termination */
  fprintf (stderr,"IOC Device ID String: %s\n", ioctl_string_shifted_by_two);

  model = epl_identify(ioctl_string_shifted_by_two);
}
