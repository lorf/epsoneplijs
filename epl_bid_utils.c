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

#include "epl_job.h"
#include "epl_bid.h"

void epl_bid_init(EPL_job_info *epl_job_info)
{

  switch (epl_job_info->connectivity)
    {
#ifdef HAVE_LIBUSB
    case VIA_LIBUSB: 
      epl_libusb_init(epl_job_info);
      break;
#endif 
#ifdef HAVE_KERNEL_DEVICE
    case VIA_KERNEL_USB: 
      epl_kernel_init(epl_job_info);
      break;
#endif 
#ifdef HAVE_KERNEL_1284
    case VIA_KERNEL_1284: 
      /* we may check the id string if we work out how... */
      break;
#endif 
#ifdef HAVE_LIBIEEE1284
    case VIA_LIBIEEE1284: 
      epl_libieee1284_init(epl_job_info);
      break;
#endif 

    default:
      fprintf(stderr,"Unknown transport method: %d\n", epl_job_info->connectivity);
      exit(1);
      break;
    }
}


void epl_bid_prejob(EPL_job_info *epl_job_info)
{
  if(epl_job_info->model == MODEL_5700L)
    {
      /* this 2-byte code is for 5700L only, careful!! */
      char byte[] = {0x00, 0x00};
      byte[0] = 0x06;
      epl_write_bid(epl_job_info, byte, 2);
      byte[0] = 0x05;
      epl_write_bid(epl_job_info, byte, 2);
    }
}

void epl_bid_mid(EPL_job_info *epl_job_info)
{
  if(epl_job_info->model == MODEL_5700L)
    {
      /* this 2-byte code is for 5700L only, careful!! */
      char byte[] = {0x07, 0x00};
      epl_write_bid(epl_job_info, byte, 2);
    }
}

void epl_bid_end(EPL_job_info *epl_job_info)
{
  switch (epl_job_info->connectivity)
    {
#ifdef HAVE_LIBUSB
    case VIA_KERNEL_USB: 
      epl_libusb_end(epl_job_info);
      break;
#endif
#ifdef HAVE_LIBIEEE1284
    case VIA_LIBIEEE1284: 
      epl_libieee1284_end(epl_job_info);
      break;
#endif 
    default:
      /* kernel device doesn't need closing? */
      break;
    }  
}

int epl_identify(char *string)
{

  if(strstr(string, "EPL-5700L"))
    {
      fprintf(stderr, "Confirmed EPL-5700L\n");
      return MODEL_5700L;
    }
  else if(strstr(string, "EPL-5800L"))
    {
      fprintf(stderr, "Confirmed EPL-5800L\n");
      return MODEL_5800L;
    }
  else if(strstr(string, "EPL-5900L"))
    {
      fprintf(stderr, "Confirmed EPL-5900L\n");
      return MODEL_5900L;
    }
  else if(strstr(string, "EPL-6100L"))
    {
      fprintf(stderr, "Confirmed EPL-6100L\n");
      return MODEL_6100L;
    }

  fprintf(stderr, "so, what printer is this?\n");
  return MODEL_UNKNOWN;
}

