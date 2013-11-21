/*
 * testlibusb.c
 *
 *  Test suite program
 */

#include <stdio.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <usb.h>

void print_endpoint(struct usb_endpoint_descriptor *endpoint);
void print_altsetting(struct usb_interface_descriptor *interface);
void print_interface(struct usb_interface *interface);
void print_configuration(struct usb_config_descriptor *config);

void print_endpoint(struct usb_endpoint_descriptor *endpoint)
{
  printf("      bEndpointAddress: %02xh\n", endpoint->bEndpointAddress);
  printf("      bmAttributes:     %02xh\n", endpoint->bmAttributes);
  printf("      wMaxPacketSize:   %d\n", endpoint->wMaxPacketSize);
  printf("      bInterval:        %d\n", endpoint->bInterval);
  printf("      bRefresh:         %d\n", endpoint->bRefresh);
  printf("      bSynchAddress:    %d\n", endpoint->bSynchAddress);
}

void print_altsetting(struct usb_interface_descriptor *interface)
{
  int i;

  printf("    bInterfaceNumber:   %d\n", interface->bInterfaceNumber);
  printf("    bAlternateSetting:  %d\n", interface->bAlternateSetting);
  printf("    bNumEndpoints:      %d\n", interface->bNumEndpoints);
  printf("    bInterfaceClass:    %d\n", interface->bInterfaceClass);
  printf("    bInterfaceSubClass: %d\n", interface->bInterfaceSubClass);
  printf("    bInterfaceProtocol: %d\n", interface->bInterfaceProtocol);
  printf("    iInterface:         %d\n", interface->iInterface);

  for (i = 0; i < interface->bNumEndpoints; i++)
    print_endpoint(&interface->endpoint[i]);
}

void print_interface(struct usb_interface *interface)
{
  int i;

  for (i = 0; i < interface->num_altsetting; i++)
    print_altsetting(&interface->altsetting[i]);
}

void print_configuration(struct usb_config_descriptor *config)
{
  int i;

  printf("  wTotalLength:         %d\n", config->wTotalLength);
  printf("  bNumInterfaces:       %d\n", config->bNumInterfaces);
  printf("  bConfigurationValue:  %d\n", config->bConfigurationValue);
  printf("  iConfiguration:       %d\n", config->iConfiguration);
  printf("  bmAttributes:         %02xh\n", config->bmAttributes);
  printf("  MaxPower:             %d\n", config->MaxPower);

  for (i = 0; i < config->bNumInterfaces; i++)
    print_interface(&config->interface[i]);
}

int main(void)
{
  struct usb_bus *bus;
  struct usb_device *dev;

  usb_init();

  usb_find_busses();
  usb_find_devices();

  printf("bus/device  idVendor/idProduct\n");
  for (bus = usb_busses; bus; bus = bus->next) {
    for (dev = bus->devices; dev; dev = dev->next) {
      int ret, i, i_stripe;
      char string[256];
      usb_dev_handle *udev;
      
      printf("%s/%s     %04X/%04X\n", bus->dirname, dev->filename,
	     dev->descriptor.idVendor, dev->descriptor.idProduct);
      
      if ((dev->descriptor.idVendor == 0x04B8) 
	  && (dev->descriptor.idProduct == 0x0001)) {
	udev = usb_open(dev);
	if (udev) {
	  int in_ep;
	  int out_ep;
	  char bytes[4096];

	  usb_set_debug(10);

	  if (dev->descriptor.iManufacturer) {
	    ret = usb_get_string_simple(udev, dev->descriptor.iManufacturer, string, sizeof(string));
	    if (ret > 0)
	      printf("- Manufacturer : %s\n", string);
	    else
	      printf("- Unable to fetch manufacturer string\n");
	  }
	  
	  if (dev->descriptor.iProduct) {
	    ret = usb_get_string_simple(udev, dev->descriptor.iProduct, string, sizeof(string));
	    if (ret > 0)
	      printf("- Product      : %s\n", string);
	    else
	      printf("- Unable to fetch product string\n");
	  }
	  
	  if (dev->descriptor.iSerialNumber) {
	    ret = usb_get_string_simple(udev, dev->descriptor.iSerialNumber, string, sizeof(string));
	    if (ret > 0)
	      printf("- Serial Number: %s\n", string);
	    else
	      printf("- Unable to fetch serial number string\n");
	  }


	    
	  ret = usb_claim_interface (udev, 0);

	  printf ("claim %d\n", ret);
	  
	  out_ep = 0x01;
	  in_ep = 0x82;

	  bytes[1] = 0x00;

	  bytes[0] = 0x06;
	  ret = usb_bulk_write (udev, out_ep,bytes, 2, 100);
	  printf ("first write %d\n", ret);
	  ret = usb_bulk_read (udev, in_ep,bytes, 19, 100);
	  printf ("first read %d\n", ret);

	  bytes[0] = 0x07;
	  ret = usb_bulk_write (udev, out_ep,bytes, 2, 100);
	  printf ("2nd write %d\n", ret);
	  ret = usb_bulk_read (udev, in_ep,bytes, 28, 100);
  	  printf ("2nd read %d\n", ret);

	  bytes[0] = 0x06;
	  ret = usb_bulk_write (udev, out_ep,bytes, 2, 100);
	  printf ("3 write %d\n", ret);
	  ret = usb_bulk_read (udev, in_ep,bytes, 19, 100);
	  printf ("3 read %d\n", ret);

	  bytes[0] = 0x05;
	  ret = usb_bulk_write (udev, out_ep,bytes, 2, 100);
	  printf ("4 write %d\n", ret);
	  ret = usb_bulk_read (udev, in_ep, bytes, 21, 100);
	  printf ("4 read %d\n", ret);

	  sprintf(bytes, "%c%c%c%c%c%c%c%c",
		  0x00, 0x00,0x00,0x00, 0x01,0x01, 0x00,0x03) ;
	  ret = usb_bulk_write (udev, out_ep,bytes, 8, 100);
	  printf ("5 write %d\n", ret);
	  ret = usb_bulk_read (udev, in_ep, bytes, 15, 100);
	  printf ("5 read %d\n", ret);

	  bytes[0] = 0x07;
	  ret = usb_bulk_write (udev, out_ep, bytes, 2, 100);
	  printf ("2nd write %d\n", ret);
	  ret = usb_bulk_read (udev, in_ep, bytes, 28, 100);
  	  printf ("2nd read %d\n", ret);

	  sprintf(bytes, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
		  0x02,0x00,0xff,0x40,0x01,0x2c,0x00,0x00,
		  0x00,0x00,0x0d,0x50,0x09,0x4b,0x00,0x36,
		  0xff,0x00,0x01,0xff,0xfe,0x00,0xd2,0x01,
		  0x29);
	  ret = usb_bulk_write (udev, out_ep,bytes, 25, 100);
	  printf ("5 write %d\n", ret);
	  ret = usb_bulk_read (udev, in_ep, bytes, 30, 100);
	  printf ("5 read %d\n", ret);

	  for(i_stripe = 0 ; i_stripe < 54 ; i_stripe++) {
	    int j;
	    sprintf(bytes, "%c%c%c%c%c%c%c",
		    0x04,  0x00, 0x01, 0x00, 0x00, 0x00, 0x68);
	    ret = usb_bulk_write (udev, out_ep,bytes, 7, 100);
	    printf ("5 write %d\n", ret);

	    for (j = 0 ; j < 4; j++) {
	      sprintf(bytes, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
		      0xa0,0x1d,
		      0x74,0x03,0x0e,0x80,0x01,0xd0,0x40,0x3a,
		      0xe8,0x07,0x1d,0x00,0x03,0xa0,0x80,0x74,
		      0xd0,0x0e,0x3a,0x01,0x07,0x40,0x00,0xe8);
	      ret = usb_bulk_write (udev, out_ep,bytes, 26, 100);
	      printf ("5 write %d\n", ret);
	    }
	  }
	  bytes[1] = 0x00;

	  bytes[0] = 0x03;
	  ret = usb_bulk_write (udev, out_ep,bytes, 2, 100);
	  printf ("3 write %d\n", ret);
	  ret = usb_bulk_read (udev, in_ep,bytes, 15, 100);
	  printf ("3 read %d\n", ret);

	  bytes[0] = 0x01;
	  ret = usb_bulk_write (udev, out_ep,bytes, 2, 100);
	  printf ("4 write %d\n", ret);
	  ret = usb_bulk_read (udev, in_ep, bytes, 15, 100);
	  printf ("4 read %d\n", ret);

  	  if (usb_release_interface (udev,0) < 0) {
	    printf ("Could not release interface %d\n", 0);
	  } else {
	    printf ("Released interface %d.\n", 0);	    
	  }  

	  usb_close (udev);
	}

	if (!dev->config) {
	  printf("  Couldn't retrieve descriptors\n");
	  continue;
	}
	
	for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
	  print_configuration(&dev->config[i]);

	

      }
    }
  }
  return 0;
}

