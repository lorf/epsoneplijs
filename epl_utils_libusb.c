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

#include "libusb/usb.h"

#include <stdlib.h>
#include <string.h>

#include "epl_job.h"
#include "epl_bid.h"

#define MAX_DEVICE_ID_SIZE     1024

void epl_libusb_init(EPL_job_info *epl_job_info)
{
  unsigned char device_id_string[MAX_DEVICE_ID_SIZE];  /* This 1024 comes from the kernel header */
  int err, length;

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

   /* USB Spec: value = 0, index=0, for Interface GetStatus()  */
	  err = usb_control_msg(udev, USB_TYPE_CLASS | USB_ENDPOINT_IN | USB_RECIP_INTERFACE , USB_REQ_GET_STATUS,
				0,0,
				device_id_string, MAX_DEVICE_ID_SIZE - 1, EPL_USB_WRITE_TIMEOUT);  
	  
	  if ( err < 0 ) {
	    fprintf (stderr,"Can't Get Device ID String: %d\n", err);  
	  } else {
	    length = (device_id_string[0] << 8) + device_id_string[1] - 2;
	    fprintf(stderr,"Device ID String: ");
	    fwrite (device_id_string + 2, 1, length, stderr);
	    fprintf (stderr,"\n");
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

void epl_libusb_end(EPL_job_info *epl_job_info)
{
  if (usb_release_interface (epl_job_info->usb_dev_hd,0) < 0) {
    printf ("Could not release interface %d\n", 0);
  } else {
    printf ("Released interface %d.\n", 0);         
  }  
  usb_close (epl_job_info->usb_dev_hd);
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
