/*
  Copyright (c) 2003 Hin-Tak Leung
*/  

/*
  Some of this code was adapted from the test program that
  came with libieee1284
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

/* strdup need this */
#ifndef _SVID_SOURCE
#define _SVID_SOURCE 1 
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>

#include "libieee1284/include/ieee1284.h"

typedef struct parport typ_parport;
typedef struct parport_list typ_parport_list;

#include "epl_job.h"
#include "epl_bid.h"

enum devid_field { devid_cls, devid_mfg, devid_mdl };

static char *field (char *id, enum devid_field f)
{
  char *result = "(?)";
  char *p = NULL;
  id += 2;
  switch (f)
    {
    case devid_cls:
      p = strstr (id, "CLASS:");
      if (!p)
	p = strstr (id, "class:");
      if (!p)
	p = strstr (id, "CLS:");
      if (!p)
	p = strstr (id, "cls:");
      break;
    case devid_mfg:
      p = strstr (id, "MANUFACTURER:");
      if (!p)
	p = strstr (id, "manufacturer:");
      if (!p)
	p = strstr (id, "MFG:");
      if (!p)
	p = strstr (id, "mfg:");
      break;
    case devid_mdl:
      p = strstr (id, "MODEL:");
      if (!p)
	p = strstr (id, "model:");
      if (!p)
	p = strstr (id, "MDL:");
      if (!p)
	p = strstr (id, "mdl:");
      break;
    }
  
  if (p)
    {
      char *q;
      char c = 0x00;
      p = strchr (p, ':') + 1;
      q = strchr (p, ';');
      if (q)
	{
	  c = *q;
	  *q = '\0';
	}
      result = strdup (p); /* leaks, but this is just a test harness */
      if (q)
	*q = c;
    }
  
  return result;
}

static void test_deviceid (EPL_job_info *epl_job_info, struct parport_list *pl)
{
  int model = 0;
  int i, j;
  printf ("Found %d ports:\n", pl->portc);
  for (i = 0; i < pl->portc; i++)
    {
      char id[500];
      printf ("  %s: ", pl->portv[i]->name);
      
      if (ieee1284_get_deviceid (pl->portv[i], -1, F1284_FRESH, id, 500) > -1)
	{
	  printf ("%s, %s %s", field (id, devid_cls), field (id, devid_mfg),
		  field (id, devid_mdl));
	  model = epl_identify(id+2); 
	  if (model > 0)  
	    {
	      epl_job_info->model = model;
	      epl_job_info->port = pl->portv[i];
	    }
	}
      else if (ieee1284_get_deviceid (pl->portv[i], -1, 0, id, 500) > -1)
	{
	  printf ("(may be cached) %s, %s %s", field (id, devid_cls),
		  field (id, devid_mfg), field (id, devid_mdl));
	  model = epl_identify(id+2); 
	  if (model > 0)  
	    {
	      epl_job_info->model = model;
	      epl_job_info->port = pl->portv[i];
	    }
	}
      printf ("\n");
      for (j = 0; j < 4; j++)
	if (ieee1284_get_deviceid (pl->portv[i], j, 0, id, 500) > -1)
	  {
	    printf ("    Daisy chain address %d: (may be cached) %s, %s %s\n", j,
		    field (id, devid_cls), field (id, devid_mfg),
		    field (id, devid_mdl));
	    model = epl_identify(id+2); 
	    if (model > 0)  
	      {
		epl_job_info->model = model;
		epl_job_info->port = pl->portv[i];
	      }
	  }
      putchar ('\n');
    }
}
  
static int show_capabilities (unsigned int cap)
{
#define CAP(x)					\
  if (cap & CAP1284_##x)			\
    printf (#x " ");
  
  CAP(RAW);
  CAP(NIBBLE);
  CAP(BYTE);
  CAP(COMPAT);
  CAP(BECP);
  CAP(ECP);
  CAP(ECPRLE);
  CAP(ECPSWE);
  CAP(EPP);
  CAP(EPPSL);
  CAP(EPPSWE);
  CAP(IRQ);
  CAP(DMA);
  putchar ('\n');
  return 0;
}

int epl_libieee1284_write(struct parport *port, char *ts, int length)
{
  int r;
  r = ieee1284_negotiate (port, M1284_COMPAT);

  if (r != E1284_OK)
    printf ("Couldn't negotiate write: %d\n", r);

  return ieee1284_compat_write (port, 0, ts, length);
}

int epl_libieee1284_read(struct parport *port, char *inbuf, int length)
{	
  int r;
  r = ieee1284_negotiate (port,M1284_NIBBLE);
  
  if (r != E1284_OK)
    printf ("Couldn't negotiate read: %d\n", r);

  r = ieee1284_nibble_read (port , F1284_NONBLOCK, inbuf, length);
  ieee1284_terminate (port);
  
  return r;
}

void epl_libieee1284_init(EPL_job_info *epl_job_info)
{
  struct parport *port;
  int cap;

  epl_job_info->port = (typ_parport *)malloc(sizeof(typ_parport));
  epl_job_info->port_list = (typ_parport_list *)malloc(sizeof(typ_parport_list));
  ieee1284_find_ports (epl_job_info->port_list, 0);
  test_deviceid (epl_job_info, epl_job_info->port_list);
  
  if (epl_job_info->port == NULL) 
    {
      printf("Didn't find any matching printer. Exiting....\n");
      exit(1);
    }
  
  port = epl_job_info->port;
  
  if (ieee1284_open (port, 0, &cap))
    {
      printf ("%s: inaccessible\n", port->name);
      exit(1);
    }
  else
    {
      int r;
      
      printf ("%s: %#lx", port->name, port->base_addr);
      if (port->hibase_addr)
	printf (" (ECR at %#lx)", port->hibase_addr);
      printf ("\n  ");
      show_capabilities (cap);
      
      r = ieee1284_claim (port);
      if (r != E1284_OK)
	printf ("Couldn't claim port: %d\n", r);
      else
	r = ieee1284_clear_irq (port, NULL);
      if (r != E1284_OK)
	{
	  if (r == E1284_NOTAVAIL) 
	    {
	      printf ("Clear IRQ: Not available on this system\n");
	    }
	  else 
	    {
	      printf ("Couldn't clear IRQ: %d\n", r);
	    }
	}
    }
}


void epl_libieee1284_end(EPL_job_info *epl_job_info)
{
  ieee1284_release (epl_job_info->port);
  ieee1284_close (epl_job_info->port);
  free(epl_job_info->port);
  ieee1284_free_ports (epl_job_info->port_list);
  free(epl_job_info->port_list);
}
