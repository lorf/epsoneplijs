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

#define USB_VENDOR_EPSON 0x04B8
#define USB_PRODUCT_5700L 0x0001
#define USB_PRODUCT_5800L 0x0005
#define USB_PRODUCT_5900L 0x0005
#define USB_PRODUCT_6100L 0x0005

/* milliseconds */
#define EPL_USB_WRITE_TIMEOUT 5000
#define EPL_USB_READ_TIMEOUT  1000

void epl_bid_init(EPL_job_info *epl_job_info);

void epl_bid_prejob(EPL_job_info *epl_job_info);

void epl_bid_mid(EPL_job_info *epl_job_info);

void epl_bid_end(EPL_job_info *epl_job_info);

int epl_bid_reply_len(int model, unsigned char code);

#ifdef HAVE_KERNEL_DEVICE
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

#ifdef HAVE_LIBIEEE1284
int epl_libieee1284_write(struct parport *port, char *ts, int length);
int epl_libieee1284_read(struct parport *port, char *ts, int length);
void epl_libieee1284_init(EPL_job_info *epl_job_info);
void epl_libieee1284_end(EPL_job_info *epl_job_info);
#endif

