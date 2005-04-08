/*
  Copyright (c) 2003 Hin-Tak Leung, Roberto Ragusa

  Adapted from IJS 0.34 ijs_server_example.c, with 
  reference to hpijs and gimp-print.

*/


/**
 * Copyright (c) 2001-2002 artofcode LLC.
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

/* fdopen requires this */
#ifndef _POSIX_C_SOURCE 
#define _POSIX_C_SOURCE  1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_KERNEL_USB_DEVICE) || defined(HAVE_KERNEL_1284)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include "ijs.h"
#include "ijs_server.h"

#include "epl_job.h"
#include "epl_bid.h"
#include "epl_time.h"

#define BUF_SIZE 4096

typedef struct _Epson_EPL_ParamList Epson_EPL_ParamList;

struct _Epson_EPL_ParamList {
  Epson_EPL_ParamList *next;
  char *key;
  char *value;
  int value_size;
};

static int
epson_epl_status_cb (void *status_cb_data,
		  IjsServerCtx *ctx,
		  IjsJobId job_id)
{
#ifdef EPL_DEBUG
  fprintf (stderr, "epson_epl_status_cb\n");
#endif
  return 0;
}

static int
epson_epl_list_cb (void *list_cb_data,
		 IjsServerCtx *ctx,
		 IjsJobId job_id,
		 char *val_buf,
		 int val_size)
{
  const char *param_list = "OutputFile,OutputFD,DeviceManufacturer,DeviceModel,PageImageFormat,Dpi,Width,Height,BitsPerSample,ByteSex,ColorSpace,NumChan,PaperSize,PrintableArea,PrintableTopLeft,TopLeft,Quality,EplDpi,EplRitech,EplDensity,EplTonerSave,EplPaperType,EplCopies";
  int size = strlen (param_list);

#ifdef EPL_DEBUG
  fprintf (stderr, "epson_epl_list_cb\n");
#endif

  if (size > val_size)
    {
#ifdef EPL_DEBUG
      fprintf (stderr, "IJS_EBUF\n");
#endif
      return IJS_EBUF;
    }
  memcpy (val_buf, param_list, size);
  return size;
}

static int
epson_epl_enum_cb (void *enum_cb_data,
		 IjsServerCtx *ctx,
		 IjsJobId job_id,
		 const char *key,
		 char *val_buf,
		 int val_size)
{
  const char *val = NULL;

#ifdef EPL_DEBUG
  fprintf (stderr, "epson_epl_enum_cb\n");
#endif

  if (!strcmp (key, "ColorSpace"))
    val = "DeviceGray";
  else if (!strcmp (key, "DeviceManufacturer"))
    val = "Epson";
  else if (!strcmp (key, "DeviceModel"))
    val = "EPL5700L,EPL5800L,EPL5900L,EPL6100L,EPL6200L";
  else if (!strcmp (key, "PageImageFormat"))
    val = "Raster";
  else if (!strcmp (key, "EplDpi"))
    val = "600,300,Class600,Class1200";
  else if (!strcmp (key, "EplRitech"))
    val = "on, off";
  else if (!strcmp (key, "EplDensity"))
    val = "3,1,2,4,5";
  else if (!strcmp (key, "EplTonerSave"))
    val = "off, on";
  else if (!strcmp (key, "EplPaperType"))
    val = "0,1,2,3";
  else if (!strcmp (key, "EplCopies"))
    val = "1,2,3,4,5,6";

  if (val == NULL)
    {
#ifdef EPL_DEBUG
      fprintf (stderr, "IJS_EUNKPARAM\n");
#endif
      return IJS_EUNKPARAM;
    }
  else
    {
      int size = strlen (val);

      if (size > val_size)
	{
#ifdef EPL_DEBUG
	  fprintf (stderr, "IJS_EBUF\n");
#endif
	  return IJS_EBUF;
	}
      memcpy (val_buf, val, size);
      return size;
    }
}

/* A C implementation of /^(\d\.+\-eE)+x(\d\.+\-eE)+$/ */
static int
epson_epl_parse_wxh (const char *val, int size,
		   double *pw, double *ph)
{
  char buf[256];
  char *tail;
  int i;

  for (i = 0; i < size; i++)
    if (val[i] == 'x')
      break;

  if (i + 1 >= size)
    {
#ifdef EPL_DEBUG
      fprintf (stderr, "IJS_ESYNTAX\n");     
#endif
      return IJS_ESYNTAX;
    }

  if (i >= sizeof(buf))
    {
#ifdef EPL_DEBUG
      fprintf (stderr, "IJS_EBUF\n");     
#endif
      return IJS_EBUF;
    }
  memcpy (buf, val, i);
  buf[i] = 0;
  *pw = strtod (buf, &tail);
  if (tail == buf)
    {
#ifdef EPL_DEBUG
      fprintf (stderr, "IJS_ESYNTAX\n");     
#endif
      return IJS_ESYNTAX;
    }
  if (size - i > sizeof(buf))
    {
#ifdef EPL_DEBUG
      fprintf (stderr, "IJS_EBUF\n");     
#endif
      return IJS_EBUF;
    }
  memcpy (buf, val + i + 1, size - i - 1);
  buf[size - i - 1] = 0;
  *ph = strtod (buf, &tail);
  if (tail == buf)
    {
#ifdef EPL_DEBUG
      fprintf (stderr, "IJS_ESYNTAX\n");     
#endif
      return IJS_ESYNTAX;
    }
  return 0;
}

/**
 * epson_epl_find_key: Search parameter list for key.
 *
 * @key: key to look up
 *
 * Return value: Epson_EPL_ParamList entry matching @key, or NULL.
 **/
static Epson_EPL_ParamList *
epson_epl_find_key (Epson_EPL_ParamList *pl, const char *key)
{
  Epson_EPL_ParamList *curs;

  for (curs = pl; curs != NULL; curs = curs->next)
    {
      if (!strcmp (curs->key, key))
	return curs;
    }
  return NULL;
}

static int
set_param (Epson_EPL_ParamList **ppl, const char *key, const char *value, int value_size)
{
  int key_len = strlen (key);
  Epson_EPL_ParamList *pl;
#ifdef EPL_DEBUG
  fprintf(stderr,"                    set_param: %s=",key);
  fwrite (value, 1, value_size, stderr);
  fputs ("\n", stderr);
#endif
  pl = epson_epl_find_key (*ppl, key);

  if (pl == NULL)
    {
      pl = (Epson_EPL_ParamList *)malloc (sizeof (Epson_EPL_ParamList));
      pl->next = *ppl;
      pl->key = malloc (key_len + 1);
      memcpy (pl->key, key, key_len + 1);
      *ppl = pl;
    }
  else
    {
      free (pl->value);
    }

  pl->value = malloc (value_size);
  memcpy (pl->value, value, value_size);
  pl->value_size = value_size;

  return 0;
}

/**
 * @printable: An array in which to store the printable area.
 *
 * On return, @printable = PrintableArea[0:1] + TopLeft[0:1]
 **/
static int
epson_epl_compute_printable (Epson_EPL_ParamList *pl, double printable[4])
{
  Epson_EPL_ParamList *curs;
  double width, height;
  int code;
  double margin = 0.167; /* The unprintable margin */

  curs = epson_epl_find_key (pl, "PaperSize");
  if (curs == NULL)
    return -1;
  code = epson_epl_parse_wxh (curs->value, curs->value_size, &width, &height);

  if (code == 0)
    {
      /* We impose symmetric margins */
      printable[0] = width - 2 * margin;
      printable[1] = height - 2 * margin;
      printable[2] = margin;
      printable[3] = margin;
    }

  return code;
}

/*
static int
epson_epl_compute_offset (Epson_EPL_ParamList *pl, IjsPageHeader *ph,
			double *px0, double *py0)
{
  Epson_EPL_ParamList *curs;
  double width, height;
  double top, left;
  int code;

  *px0 = 0;
  *py0 = 0;

  curs = epson_epl_find_key (pl, "PaperSize");
  if (curs == NULL)
    return -1;

  code = epson_epl_parse_wxh (curs->value, curs->value_size, &width, &height);

  if (code == 0)
    {
      curs = epson_epl_find_key (pl, "TopLeft");
      if (curs != NULL)
	{
	  code = epson_epl_parse_wxh (curs->value, curs->value_size,
				    &top, &left);
	}
      else
	{
	  double printable[4];

	  code = epson_epl_compute_printable (pl, printable);
	  if (code == 0)
	    {
	      top = printable[2];
	      left = printable[3];
	    }
	}
    }

  if (code == 0)
    {
      *px0 = left;
      *py0 = height - ph->height / ph->yres - top;
    }

  return code;
}
*/

static int
epson_epl_get_cb (void *get_cb_data,
		 IjsServerCtx *ctx,
		 IjsJobId job_id,
		 const char *key,
		 char *val_buf,
		 int val_size)
{
  Epson_EPL_ParamList *pl = *(Epson_EPL_ParamList **)get_cb_data;
  Epson_EPL_ParamList *curs;
  const char *val = NULL;
  char buf[256];
  int code;

#ifdef EPL_DEBUG
  fprintf (stderr, "epson_epl_get_cb: %s\n", key);
#endif
  curs = epson_epl_find_key (pl, key);
  if (curs != NULL)
    {
      if (curs->value_size > val_size)
	{
#ifdef EPL_DEBUG
	  fprintf (stderr, "IJS_EBUF\n");     	  
#endif
	  return IJS_EBUF;
	}
      memcpy (val_buf, curs->value, curs->value_size);
      return curs->value_size;
    }

  if (!strcmp (key, "PrintableArea") || !strcmp (key, "PrintableTopLeft"))
    {
      double printable[4];
      int off = !strcmp (key, "PrintableArea") ? 0 : 2;

      code = epson_epl_compute_printable (pl, printable);
      if (code == 0)
	{
	  sprintf (buf, "%gx%g", printable[off + 0], printable[off + 1]);
	  val = buf;
	}
    }

  if (!strcmp (key, "DeviceManufacturer"))
    val = "Epson";
  else if (!strcmp (key, "DeviceModel"))
    val = "undefined";	/* this *has* to be set from the client */
  else if (!strcmp (key, "PageImageFormat"))
    val = "Raster";
  else if (!strcmp (key, "Dpi"))
    val = "600x600";

  if (val == NULL)
    {
#ifdef EPL_DEBUG
      fprintf (stderr, "epson_epl_get_cb: Failed key %s\n", key);
#endif
      return IJS_EUNKPARAM;
    }
  else
    {
      int size = strlen (val);

#ifdef EPL_DEBUG
      fprintf (stderr, "epson_epl_get_cb: Success key = %s, val = %s\n", 
	       key,
	       val);
#endif
      if (size > val_size)
	{
#ifdef EPL_DEBUG
	  fprintf (stderr, "IJS_EBUF\n");     
#endif
	  return IJS_EBUF;
	}
      memcpy (val_buf, val, size);
      return size;
    }
}

static int
epson_epl_set_cb (void *set_cb_data, IjsServerCtx *ctx, IjsJobId job_id,
		const char *key, const char *value, int value_size)
{
  Epson_EPL_ParamList **ppl = (Epson_EPL_ParamList **)set_cb_data;
  int code;
  char buf[256];

#ifdef EPL_DEBUG
  fprintf (stderr, "epson_epl_set_cb: key:'%s',value:'", key);
  fwrite (value, 1, value_size, stderr);
  fputs ("'\n", stderr);
#endif

  /* Convert value to a zero-terminated string */
  if( value_size >= sizeof(buf)){
#ifdef EPL_DEBUG
    fprintf (stderr, "IJS_EBUF\n");
    return IJS_EBUF;
#endif
  }
  memcpy (buf, value, value_size);
  buf[value_size] = 0;

  /* Handle side effects of parameters */

  if (!strcmp (key, "PaperSize"))
    {
      /* Setting PaperSize affects PrintableArea, which is used by ghostscript */
      double width, height;

      code = epson_epl_parse_wxh (value, value_size, &width, &height);
      if (code < 0)
	return code;
    }

  if (!strcmp (key, "EplDpi"))
    {
      /* Setting EplDpi affects Dpi, which is used by ghostscript */
      char *s;
      if(!strcmp (buf, "300"))
        {
          s = "300x300";
	}
      else if(!strcmp (buf, "Class600"))
        {
          s = "600x300";
	}
      else if(!strcmp (buf, "600"))
        {
          s = "600x600";
	}
      else if(!strcmp (buf, "Class1200"))
        {
          s = "1200x600";
	}
      else
        {
          return IJS_ERANGE;
	}
        code = set_param (ppl, "Dpi", s, strlen (s));
        if (code < 0)
	  return code;	/* because we can't go on setting EplDpi */
    }

  if (!strcmp (key, "PrintableArea"))
    {
      /* This can't be set */
      fprintf (stderr, "Setting %s is not allowed\n", key);
      return IJS_ERANGE;
    }

  if (!strcmp (key, "PrintableTopLeft"))
    {
      /* This can't be set */
      fprintf (stderr, "Setting %s is not allowed\n", key);
      return IJS_ERANGE;
    }

  /* Update parameter list */
  
  code = set_param (ppl, key, value, value_size);
  if (code < 0)
    return code;
  return 0;
}

/**
 * Finds a parameter in the param list, and allocates a null terminated
 * string with the value.
 **/
static char *
find_param (Epson_EPL_ParamList *pl, const char *key)
{
  Epson_EPL_ParamList *curs;
  char *result;

  curs = epson_epl_find_key (pl, key);
  if (curs == NULL)
    return NULL;

  result = malloc (curs->value_size + 1);
  memcpy (result, curs->value, curs->value_size);
  result[curs->value_size] = 0;
  return result;
}

static void
free_param_list (Epson_EPL_ParamList *pl)
{
  Epson_EPL_ParamList *next;

  for (; pl != NULL; pl = next)
    {
      next = pl->next;
      free (pl->key);
      free (pl->value);
      free (pl);
    }
}

static int
pl_to_epljobinfo (Epson_EPL_ParamList *pl, IjsPageHeader ph, EPL_job_info *epl_job_info)
{
  char *fn;
  char *s;

  /* Check options */

#ifdef EPL_DEBUG
  fprintf(stderr, "settings:\n");
#endif

  if (epl_job_info->outfile == NULL)
    {
      fn = find_param (pl, "OutputFile");
      /* todo: check error! */
	  
      if (fn == NULL)
        {
          fn = find_param (pl, "OutputFD");
          if (fn != NULL)
  	    {
	    epl_job_info->outfile = fdopen (atoi (fn), "w");
	    }
        }
      else
        {
          epl_job_info->outfile = fopen (fn, "w");
        }
      if (epl_job_info->outfile == NULL)
        {
          fprintf (stderr, "can't open output file %s\n", fn);
          fclose (stdin);
          fclose (stdout);
          return 1;
        }
      if (fn != NULL)
        free (fn);
    }

  /* DeviceModel */
  s = find_param(pl, "DeviceModel");
  if (s == NULL)
    {
      fprintf(stderr, "Printer DeviceModel not set, aborting!\n");
      return 1;
    }

  if (strcmp(s, "EPL5700L") == 0)
    {
      epl_job_info->model = MODEL_5700L ;
    }
  else if (strcmp(s, "EPL5800L") == 0)
    {
      epl_job_info->model = MODEL_5800L ;
    }
  else if (strcmp(s, "EPL5900L") == 0)
    {
      epl_job_info->model = MODEL_5900L ;
    }
  else if (strcmp(s, "EPL6100L") == 0)
    {
      epl_job_info->model = MODEL_6100L ;
    }
  else if (strcmp(s, "EPL6200L") == 0)
    {
      epl_job_info->model = MODEL_6200L ;
    }
  else 
    {
      fprintf(stderr, "Unknown Printer %s, aborting!\n", s);
      return 1;
    }
#ifdef EPL_DEBUG
  fprintf(stderr, "  Using protocol for %s\n",printername[epl_job_info->model]);
#endif

  /* PaperSize */
  s = find_param(pl, "PaperSize");
  if (s == NULL)
    {
      fprintf(stderr, "PaperSize not set, using default (A4 210x297mm)\n");
      epl_job_info->paper_size_mm_h = 210;
      epl_job_info->paper_size_mm_v = 297;
    }
  else
    {
      double width, height;
      int code;
      code = epson_epl_parse_wxh (s, strlen(s), &width, &height);
      if (code == 0)
	{
	  epl_job_info->paper_size_mm_h = (int)(width * 25.4 + 0.5);
	  epl_job_info->paper_size_mm_v = (int)(height * 25.4 + 0.5);
	}
      else
	{
          fprintf(stderr, "Unparsable PaperSize %s, aborting!\n", s);
          return 1;
	}
#ifdef EPL_DEBUG
      fprintf(stderr, "  Papersize is %s inches\n", s);
#endif
    }


  /* EplDpi */
  s = find_param(pl, "EplDpi");
  if (s == NULL)
    {
      fprintf(stderr, "EplDpi not set, using default (600)\n");
      epl_job_info->dpi_h = 600;
      epl_job_info->dpi_v = 600;
    }
  else if (strcmp(s, "300") == 0)
    {
      epl_job_info->dpi_h = 300;
      epl_job_info->dpi_v = 300;
    }
  else if (strcmp(s, "Class600") == 0)
    {
      epl_job_info->dpi_h = 600;
      epl_job_info->dpi_v = 300;
    }
  else if (strcmp(s, "600") == 0)
    {
      epl_job_info->dpi_h = 600;
      epl_job_info->dpi_v = 600;
    }
  else if (strcmp(s, "Class1200") == 0)
    {
      epl_job_info->dpi_h = 1200;
      epl_job_info->dpi_v = 600;
    }
  else 
    {
      fprintf(stderr, "Unknown EplDpi value %s, aborting!\n", s);
      return 1;
    }
#ifdef EPL_DEBUG
  fprintf(stderr, "  Printing at DPI %s\n", s);
#endif

  /* EplRitech */
  s = find_param(pl, "EplRitech");
  if (s == NULL)
    {
      fprintf(stderr, "EplRitech not set, using default (on)\n");
      epl_job_info->ritech = 1;
    }
  else if (strcmp(s, "on") == 0)
    {
      epl_job_info->ritech = 1;
    }
  else if (strcmp(s, "off") == 0)
    {
      epl_job_info->ritech = 0;
    }
  else 
    {
      fprintf(stderr, "Unknown EplRitech value %s, aborting!\n", s);
      return 1;
    }
#ifdef EPL_DEBUG
  fprintf(stderr, "  Ritech is %s\n", s);
#endif

  /* EplDensity */
  s = find_param(pl, "EplDensity");
  if (s == NULL)
    {
      fprintf(stderr, "EplDensity not set, using default (3)\n");
      epl_job_info->density = 3;
    }
  else if (strcmp(s, "1") == 0)
    {
      epl_job_info->density = 1;
    }
  else if (strcmp(s, "2") == 0)
    {
      epl_job_info->density = 2;
    }
  else if (strcmp(s, "3") == 0)
    {
      epl_job_info->density = 3;
    }
  else if (strcmp(s, "4") == 0)
    {
      epl_job_info->density = 4;
    }
  else if (strcmp(s, "5") == 0)
    {
      epl_job_info->density = 5;
    }
  else 
    {
      fprintf(stderr, "Unknown EplDensity value %s, aborting!\n", s);
      return 1;
    }
#ifdef EPL_DEBUG
  fprintf(stderr, "  Density is %s\n", s);
#endif

  /* EplTonerSave */
  s = find_param(pl, "EplTonerSave");
  if (s == NULL)
    {
      fprintf(stderr, "EplTonerSave not set, using default (off)\n");
      epl_job_info->toner_save = 0;
    }
  else if (strcmp(s, "on") == 0)
    {
      epl_job_info->toner_save = 1;
    }
  else if (strcmp(s, "off") == 0)
    {
      epl_job_info->toner_save = 0;
    }
  else 
    {
      fprintf(stderr, "Unknown EplTonerSave value %s, aborting!\n", s);
      return 1;
    }
#ifdef EPL_DEBUG
  fprintf(stderr, "  TonerSave is %s\n", s);
#endif

  /* EplPaperType */
  s = find_param(pl, "EplPaperType");
  if (s == NULL)
    {
      fprintf(stderr, "EplPaperType not set, using default Normal (0)\n");
      epl_job_info->papertype = 0;
    }
  else if ((strcmp(s, "0") == 0) || (strcmp(s, "Normal") == 0))
    {
      epl_job_info->papertype = 0;
    }
  else if ((strcmp(s, "1") == 0) || (strcmp(s, "Thick") == 0))
    {
      epl_job_info->papertype = 1;
    }
  else if ((strcmp(s, "2") == 0) || (strcmp(s, "Thicker") == 0))
    {
      epl_job_info->papertype = 2;
    }
  else if ((strcmp(s, "3") == 0) || (strcmp(s, "Transparency") == 0))
    {
      epl_job_info->papertype = 3;
    }
  else 
    {
      fprintf(stderr, "Unknown EplPaperType value %s, aborting!\n", s);
      return 1;
    }
#ifdef EPL_DEBUG
  fprintf(stderr, "  PaperType is %s\n", s);
#endif

  /* EplCopies */
  s = find_param(pl, "EplCopies");
  if (s == NULL)
    {
      fprintf(stderr, "EplCopies not set, using default (1)\n");
      epl_job_info->copies = 1;
    }
  else 
    {
      epl_job_info->copies = atoi(s);
      if(!epl_job_info->copies)
	{
	  fprintf(stderr, "Unknown EplCopies value %s, aborting!\n", s);
	  return 1;
	}
      else
	{
	  fprintf(stderr, "Printing %d copies\n", epl_job_info->copies);
	}
    }

  /* Number of channels */
  if ((ph.n_chan != 1) && (ph.n_chan != 3))
    {
      fprintf(stderr, "Number of channels is %i (unsupported), aborting!\n",
              ph.n_chan);
      return 1;
    }

  /* Bits per sample */
  if ((ph.bps != 1) && (ph.bps != 8))
    {
      fprintf(stderr, "Bit per sample is %i (unsupported), aborting!\n",
              ph.bps);
      return 1;
    }

  /* BitMap Size */
  epl_job_info->pixel_h = ph.width;
  epl_job_info->pixel_v = ph.height;
  fprintf(stderr, "  Bitmap size %ix%i (%gx%g in)\n",
          epl_job_info->pixel_h,
	  epl_job_info->pixel_v,
	  (double)epl_job_info->pixel_h / epl_job_info->dpi_h,
	  (double)epl_job_info->pixel_v / epl_job_info->dpi_v);

  /* Dpi of the bitmap */
  if (ph.xres != (double)epl_job_info->dpi_h
      || ph.yres != (double)epl_job_info->dpi_v)
    {
       fprintf(stderr, "Dpi of bitmap (%gx%g) is different from selected dpi (%gx%g)",
               ph.xres,
	       (double)epl_job_info->dpi_h,
               ph.yres,
	       (double)epl_job_info->dpi_v);
       return 1;
    }

  /* Flow Control */
  s = find_param(pl, "EplFlowControl");
  if (s == NULL)
    {
      epl_job_info->connectivity = VIA_STDOUT_PIPE;
    }
  else if (strcmp(s, "off") == 0)
    {
      epl_job_info->connectivity = VIA_STDOUT_PIPE;
    }
  else if (strcmp(s, "nowhere") == 0)
    {
      epl_job_info->connectivity = VIA_NOWHERE;
    }
#ifdef HAVE_LIBUSB
  else if (strcmp(s, "libusb") == 0)
    {
      fprintf(stderr, "Using libusb\n");
      epl_job_info->connectivity = VIA_LIBUSB;
    }
#endif
#ifdef HAVE_LIBIEEE1284
  else if (strcmp(s, "libieee1284") == 0)
    {
      fprintf(stderr, "Using libieee1284\n");
      epl_job_info->connectivity = VIA_LIBIEEE1284;
    }
#endif
#ifdef HAVE_KERNEL_USB_DEVICE
  /* We catch both /dev/usb/lp[0-9] and /dev/usblp[0-9] */
  else if (strncmp(s, "/dev/usb",8) == 0)   
    {
      fprintf(stderr, "Using kernel usb device\n");
      if (epl_job_info->connectivity != VIA_KERNEL_USB) 
	{
	  epl_job_info->connectivity = VIA_KERNEL_USB;
	  epl_job_info->kernel_fd = open(s, O_RDWR |O_SYNC);
	  if (epl_job_info->kernel_fd < 0) 
	    {
	      fprintf(stderr, "Can't open device %s, aborting!\n", s);
	      perror("Kernel device");
	      return 1;
	    }    				          
	}
    }
#endif
#ifdef HAVE_KERNEL_1284
  /* We catch /dev/lp[0-9] */
  else if (strncmp(s, "/dev/lp",7) == 0)   
    {
      fprintf(stderr, "Using kernel parallel port device\n");
      if (epl_job_info->connectivity != VIA_KERNEL_1284) 
	{
	  epl_job_info->connectivity = VIA_KERNEL_1284;
	  epl_job_info->kernel_fd = open(s, O_RDWR |O_SYNC);
	  if (epl_job_info->kernel_fd < 0) 
	    {
	      fprintf(stderr, "Can't open device %s, aborting!\n", s);
	      perror("Kernel device");
	      return 1;
	    }    				          
	}
    }
#endif
  else 
    {
      fprintf(stderr, "Unknown FlowControl option %s, aborting!\n", s);
      return 1;
    }

  /* If we get here, all it's OK */
  return 0;
}

static void
epljobinfo_cleanup (EPL_job_info *epl_job_info)
{
  if (epl_job_info->outfile)
    fclose (epl_job_info->outfile);
}

int
main (int argc, char **argv)
{
  IjsServerCtx *ctx;
  IjsPageHeader ph;
  int status;
  Epson_EPL_ParamList *pl = NULL;
  EPL_job_info *epl_job_info = NULL;
  int epl_job_started = EPL_JOB_STARTED_NO;
  int current_page_number = 1;
  
  if (argc > 1) /* the IJS plugin is being run stand-alone */ 
    {
      /* we'll try to get status if the plugin is being run stand-alone */
      epl_status(argc, argv);
      exit(0);
    } 

  ctx = ijs_server_init ();
  if (ctx == NULL)
    return (1);
  ijs_server_install_status_cb (ctx, epson_epl_status_cb, &pl);
  ijs_server_install_list_cb (ctx, epson_epl_list_cb, &pl);
  ijs_server_install_enum_cb (ctx, epson_epl_enum_cb, &pl);
  ijs_server_install_set_cb (ctx, epson_epl_set_cb, &pl);
  ijs_server_install_get_cb (ctx, epson_epl_get_cb, &pl);
  
#ifdef EPL_DEBUG
  fprintf(stderr, "Try to allocate %i byte of memory for job info\n",sizeof(EPL_job_info));
#endif
  
  epl_job_info = (EPL_job_info *)calloc(1, sizeof(EPL_job_info));
  
  if (epl_job_info == NULL) {
#ifdef EPL_DEBUG
    fprintf(stderr, "Job info memory allocation failed\n");
#endif
    exit(1);
  }

  /* The file handle has a danger of being used before initialized */
  epl_job_info->connectivity = PRE_INIT;
  epl_job_info->outfile = NULL;

  do 
    {
      int total_bytes, bytes_left;
      Epson_EPL_ParamList *curs;
      
      char *ptr_row_prev;
      char *ptr_row_current;
      
      int bytes_per_row;
      int bytes_per_row_padded;
      int total_stripes;
      int i_stripe;

      typ_stream *stream;
      
      fprintf (stderr, "getting page header\n");
      
      status = ijs_server_get_page_header (ctx, &ph);
      
      if (status) 
	{
	  if (status < 0)
	    {
	      fprintf(stderr, "ijs_server_get_page_header failed: %d\n",
		      status);
	    } else {
#ifdef EPL_DEBUG
	      fprintf(stderr, "ijs_server_get_page_header: %d - last page\n",
		      status);
#endif
	    }
	  break;
	}
      
      fprintf (stderr, "Received page %d: width %d x height %d\n",
	       current_page_number, ph.width, ph.height);
      current_page_number++;
      
      /* Before starting, dump IJS parameters */

      for (curs = pl; curs != NULL; curs = curs->next)
	{
	  fprintf (stderr, "%% IJS parameter: %s = ", curs->key);
	  fwrite (curs->value, 1, curs->value_size, stderr);
	  fputs ("\n", stderr);
	}

      /* Convert parameters to internal format */

      status = pl_to_epljobinfo(pl, ph, epl_job_info);
      if (status != 0)
        {
          fprintf (stderr, "parameters conversion failed, aborting\n");
	  exit (1);
        }

      /* Job Header */      
      
      if (epl_job_started == EPL_JOB_STARTED_NO) /* only do one job header per job */
	{
#ifdef USE_FLOW_CONTROL
          /* initialize values */
	  epl_job_info->printer_total_mem = TOTAL_MEM_DEFAULT_VALUE; /* fake */
	  epl_job_info->free_mem_last_update = epl_job_info->printer_total_mem; /* fake */
	  epl_job_info->bytes_sent_after_last_update = 0;
	  epl_job_info->stripes_sent_after_last_update = 0;
#endif
	  /* init bid if needed */
          if ((epl_job_info->connectivity != VIA_STDOUT_PIPE))
            {
              epl_bid_init(epl_job_info);
              epl_bid_prejob(epl_job_info);
            }

          status = epl_job_header (epl_job_info);      
          if (status != 0)
            {
              fprintf (stderr, "output error\n");
	      exit (1);
            }
#ifdef USE_FLOW_CONTROL
          /* initialize time_last_write_stripe */
          epl_job_info->time_last_write_stripe = get_time_now();
          if ((epl_job_info->connectivity != VIA_STDOUT_PIPE))
            {
              /* try to get info about memory */
              epl_poll(epl_job_info, 0);
              epl_poll(epl_job_info, 1); /* just for fun */
            }
#endif
	  epl_job_started = EPL_JOB_STARTED_YES ;	
        }
      
      /* Page header */
      
      if ((epl_job_info->connectivity != VIA_STDOUT_PIPE))
        {
          epl_bid_mid(epl_job_info);
        }

      status = epl_page_header(epl_job_info);
      if (status != 0)
        {
          fprintf (stderr, "output error\n");
          exit (1);
        }
      
      bytes_per_row = (ph.n_chan * ph.bps * ph.width + 8 - 1) / 8 ;
      total_bytes = bytes_per_row * ph.height;

      bytes_per_row_padded = (bytes_per_row + 3) & ~0x03;
      
#ifdef STRIPE_OVERFLOW_WORKAROUND_STRIPE_PAD
      bytes_per_row_padded += 2 ;
#endif

      /* last one may have to be padded */
      total_stripes = (ph.height + 64 - 1) / 64;
      
      stream = (typ_stream *)malloc(sizeof(typ_stream)); 
      /* 25% + 2, + 1 for rounding error */
      stream->start = (char *)malloc(bytes_per_row_padded * 64 * 5 / 4 + 2 + 1);

      ptr_row_prev = (char *)malloc(bytes_per_row_padded);
      ptr_row_current = (char *)malloc(bytes_per_row_padded);

      bytes_left = total_bytes;
      
      for (i_stripe = 0 ; i_stripe < total_stripes ; i_stripe++)
	{
	  int i, i_row;

	  stream_init(stream);
	  
	  /* clear both rows at stripe start */
	  for (i = 0 ; i < bytes_per_row_padded ; i++)
	    {
	      *(ptr_row_current + i) = (char) 0xff;
	      *(ptr_row_prev + i) = (char) 0xff;
	    }
	  
	  for (i_row = 0 ; i_row < 64 ; i_row++)
	    {
	      /* Page body */
	      
	      char *ptr_temp;
	      
	      if (bytes_left > 0)		/* no padding situation */
	        {
#ifdef EPL_DEBUG
	          fprintf (stderr, "stripe %d, row %d\n", i_stripe, i_row);
#endif	      

#ifdef VERBOSE
	          fprintf (stderr, "%d bytes left, reading %d\n", bytes_left, bytes_per_row);
#endif	      
	          status = ijs_server_get_data (ctx, ptr_row_current, bytes_per_row);
	          bytes_left -= bytes_per_row;

	          if (status)
		    {
		      fprintf (stderr, "page aborted!\n");
		      break;
		    }
                }
	      else
	        {
#ifdef EPL_DEBUG
	          fprintf (stderr, "stripe %d, row %d (padding)\n", i_stripe, i_row);
#endif	      
		  memcpy (ptr_row_current, ptr_row_prev, bytes_per_row_padded); /* repeat last line */
		}
	      
	      epl_compress_row(stream, 
			       ptr_row_current, 
			       ptr_row_prev, bytes_per_row_padded);	      
	      ptr_temp = ptr_row_current;
	      ptr_row_current = ptr_row_prev;
	      ptr_row_prev = ptr_temp;
	    }
	  stream_pad16bit(stream);

#ifdef USE_FLOW_CONTROL
	  epl_permission_to_write_stripe(epl_job_info);
#endif
	  epl_print_stripe(epl_job_info, stream, i_stripe);
          epl_job_info->time_last_write_stripe = get_time_now();
	  epl_job_info->bytes_sent_after_last_update += stream->count;
	  epl_job_info->stripes_sent_after_last_update++;
	}
	
      /* Page footer */

      status = epl_page_footer (epl_job_info);
      if (status != 0)
        {
          fprintf (stderr, "output error\n");
          exit (1);
        }
      
    }
  while (status == 0);
  
  /* Job footer */

  if (epl_job_started == EPL_JOB_STARTED_YES)
    {
      status = epl_job_footer (epl_job_info);      
      if (status != 0)
        {
          fprintf (stderr, "output error\n");
          exit (1);
        }
      if ((epl_job_info->connectivity != VIA_STDOUT_PIPE))
        {
          epl_bid_end(epl_job_info);
        }
    }

  if (status > 0) status = 0; /* normal exit */
  
  epljobinfo_cleanup (epl_job_info);
  ijs_server_done (ctx);
  
  free_param_list (pl);
  
#ifdef VERBOSE
  fprintf (stderr, "server exiting with status %d\n", status);
#endif
  return status;
}

