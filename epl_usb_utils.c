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

#ifdef HAVE_KERNEL_DEVICE
#include <sys/ioctl.h>
#endif

#ifdef HAVE_LIBUSB
#include "libusb/usb.h"
#endif

#include <string.h>

#include "epl_job.h"
#include "epl_usb.h"
#include "epl_usb_replies.h"

#ifdef HAVE_KERNEL_DEVICE
/* Copied somewhat from /usr/src/linux/drivers/usb/printer.c: */     
#define MAX_DEVICE_ID_SIZE     1024
#define LPIOC_GET_DEVICE_ID    _IOC(_IOC_READ ,'P',1,MAX_DEVICE_ID_SIZE)     
#define LPIOC_GET_PROTOCOLS    _IOC(_IOC_READ ,'P',2,sizeof(int[2]))
#define LPIOC_SET_PROTOCOL     _IOC(_IOC_WRITE,'P',3,0)                 
#define LPIOC_HP_SET_CHANNEL   _IOC(_IOC_WRITE,'P',4,0)         
#define LPIOC_GET_BUS_ADDRESS  _IOC(_IOC_READ ,'P',5,sizeof(int[2]))
#define LPIOC_GET_VID_PID      _IOC(_IOC_READ ,'P',6,sizeof(int[2]))
void epl_kernel_init(EPL_job_info *epl_job_info);
#endif

#ifdef HAVE_LIBUSB
void epl_libusb_init(EPL_job_info *epl_job_info);
void find_endpoint(EPL_job_info *epl_job_info, 
		   struct usb_endpoint_descriptor *endpoint);
void find_altsetting(EPL_job_info *epl_job_info,
		     struct usb_interface_descriptor *interface);
void find_interface(EPL_job_info *epl_job_info,
		    struct usb_interface *interface);
void find_configuration(EPL_job_info *epl_job_info, 
			struct usb_config_descriptor *config);
#endif


void epl_usb_init(EPL_job_info *epl_job_info)
{

#ifdef HAVE_LIBUSB
  if(epl_job_info->connectivity == VIA_LIBUSB) 
    {
      epl_libusb_init(epl_job_info);
    }
#endif 

#ifdef HAVE_KERNEL_DEVICE
  if(epl_job_info->connectivity == VIA_KERNEL_USB) 
    {
      epl_kernel_init(epl_job_info);
    }
#endif 

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

void epl_usb_mid(EPL_job_info *epl_job_info)
{
  if(epl_job_info->model == MODEL_5700L)
    {
      /* this 2-byte code is for 5700L only, careful!! */
      char byte[] = {0x07, 0x00};
      epl_write_bid(epl_job_info, byte, 2);
    }
}

void epl_usb_end(EPL_job_info *epl_job_info)
{
#ifdef HAVE_LIBUSB
  if (usb_release_interface (epl_job_info->usb_dev_hd,0) < 0) {
    printf ("Could not release interface %d\n", 0);
  } else {
    printf ("Released interface %d.\n", 0);         
  }  
  usb_close (epl_job_info->usb_dev_hd);
#endif
}

/**
 *
 * Internal USB routines for scanning the USB bus
 * 
**/

#ifdef HAVE_KERNEL_DEVICE
void epl_kernel_init(EPL_job_info *epl_job_info)
{
  /* the kernel device doesn't need init, but we'll do some check anyway */
  int ioctl_args[2];
  unsigned char ioctl_string[MAX_DEVICE_ID_SIZE+1];  /* This 1024 comes from the kernel header, we add 1 for null */
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
  ioctl_string[2 + length] = 0; /* string termination */
  fprintf (stderr,"IOC Device ID String: %s\n", &ioctl_string[2]);
/*
  fwrite (ioctl_string + 2, 1, length, stderr);
  fprintf (stderr,"\n");
*/
  if(strstr((char *)&ioctl_string[2], "5700L"))
    {
      fprintf(stderr, "so this is a 5700L\n");
    }
  else if(strstr((char *)&ioctl_string[2], "5800L"))
    {
      fprintf(stderr, "so this is a 5800L\n");
    }
  else if(strstr((char *)&ioctl_string[2], "5900L"))
    {
      fprintf(stderr, "so this is a 5900L\n");
    }
  else if(strstr((char *)&ioctl_string[2], "6100L"))
    {
      fprintf(stderr, "so this is a 6100L\n");
    }
  else
    {
      fprintf(stderr, "so, what printer is this?\n");
    }
}
#endif


#ifdef HAVE_LIBUSB   

void epl_libusb_init(EPL_job_info *epl_job_info)
{
  struct usb_bus *bus;
  struct usb_device *dev;

  usb_init();

  usb_find_busses();
  usb_find_devices();

#ifdef EPL_DEBUG
  usb_set_debug(10);
#endif  

  fprintf(stderr,"bus/device  idVendor/idProduct\n");

  for (bus = usb_busses; bus; bus = bus->next) {
    for (dev = bus->devices; dev; dev = dev->next) {
      int ret, i, found;
      usb_dev_handle *udev;
      
      fprintf(stderr, "%s/%s     %04X/%04X\n", 
	      bus->dirname, dev->filename,
	      dev->descriptor.idVendor, dev->descriptor.idProduct);
      
      found = 0;
      if ((dev->descriptor.idVendor == USB_VENDOR_EPSON) 
          && (dev->descriptor.idProduct == USB_PRODUCT_5700L)) {
	fprintf (stderr,"Found 5700L\n");
	found = 1;
      }
      if ((dev->descriptor.idVendor == USB_VENDOR_EPSON) 
          && (dev->descriptor.idProduct == USB_PRODUCT_5800L)) { /* and 5900L, 6100L */
	fprintf (stderr,"Found 5800L/5900L/6100L\n");
        found = 1;
      }
      if (found) {
        udev = usb_open(dev);
        if (udev) {
	  epl_job_info->usb_dev_hd = udev;
	  
	  ret = usb_claim_interface (udev, 0);
	  
	  if (ret < 0) 
	    {
	      fprintf (stderr,"Can't claim device, Check FAQ: %d\n", ret);
	      exit(1);
	    }

	  epl_job_info->usb_in_ep = -1;
	  epl_job_info->usb_out_ep = -1;

	  for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
	    find_configuration(epl_job_info, &dev->config[i]); 
	  
	  if ((epl_job_info->usb_in_ep == -1)
	      || (epl_job_info->usb_out_ep == -1))
	    {
	      fprintf (stderr,"Can't find one of the USB endpoints\n");
	      exit(1);
	    }
	}
      }
    }
  }
}

void find_endpoint(EPL_job_info *epl_job_info, 
		   struct usb_endpoint_descriptor *endpoint)
{


  if ((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) ==  USB_ENDPOINT_IN &&
      (endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK)
    {
    epl_job_info->usb_in_ep = endpoint->bEndpointAddress ;
    fprintf(stderr, "Setting inbound endpoint to %2X\n", 
	    endpoint->bEndpointAddress);

    }
  if ((endpoint->bEndpointAddress & USB_ENDPOINT_DIR_MASK) ==  USB_ENDPOINT_OUT &&
      (endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK)
    {
    epl_job_info->usb_out_ep = endpoint->bEndpointAddress ;
    fprintf(stderr, "Setting outbound endpoint to %2X\n", 
	    endpoint->bEndpointAddress);
    }
}

void find_altsetting(EPL_job_info *epl_job_info,
		     struct usb_interface_descriptor *interface)
{
  int i;

  for (i = 0; i < interface->bNumEndpoints; i++)
    find_endpoint(epl_job_info, &interface->endpoint[i]);
}
void find_interface(EPL_job_info *epl_job_info,
		    struct usb_interface *interface)
{
  int i;

  for (i = 0; i < interface->num_altsetting; i++)
    find_altsetting(epl_job_info, &interface->altsetting[i]);
}

void find_configuration(EPL_job_info *epl_job_info, 
			struct usb_config_descriptor *config)
{
  int i;

  for (i = 0; i < config->bNumInterfaces; i++)
    find_interface(epl_job_info, &config->interface[i]);
}
#endif
