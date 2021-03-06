/**
 * Copyright (c) 2003 Hin-Tak Leung, Roberto Ragusa
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


/**
 *
 * The following options enable or disable parts of the code.
 * Please be sure to understand the exact meaning of an option before
 * changing it.
 *
**/


/**
 * EPL_DEBUG (default off)
 *
 * This options activates debug output to stderr.
 *
**/
#define noEPL_DEBUG


/**
 * STRIPE_OVERFLOW_WORKAROUND (default on)
 *
 * This option activates a workaround for an apparent bug in the
 * firmware of the printers (printing a dithered photo with a wideness
 * larger than 80% of the paper size can crash the printer). There are
 * no particular disadvantages in having this enabled. Epson drivers for
 * Windows/Mac have no such option (yes, they may crash the printer).
**/
#define STRIPE_OVERFLOW_WORKAROUND_HEADER
#define STRIPE_OVERFLOW_WORKAROUND_STRIPE_PAD


/**
 * USE_DELTA_STRATEGY (default off)
 *
 * This option activates additional code which produces smaller spool
 * files but uses more CPU time (on the host). You can try it if
 * you have a relatively fast (>50MHz) machine. When this option
 * is enabled, spool files are very similar to those generated by
 * Epson drivers for Windows/Mac; the default is off (bigger files).
**/
#undef USE_DELTA_STRATEGY


/**
 * COARSE_HORIZONTAL_POSITION (default on)
 *
 * This option is only meaningful if the TopLeft IJS command is used
 * to place the bitmap at generic coordinates on paper (different from
 * PrintableTopLeft), but there is no known client using this feature
 * at the moment (ghostscript doesn't). Enabling this can cause a
 * very small misplacement of the bitmap in the horizontal
 * direction (4 pixel max, which is less than 0.34mm at 300 dpi and
 * less than 0.09mm at 1200 dpi), but reduces the CPU load."
 * (NOTE: code to support generic coordinates has not been integrated
 * yet)
**/
#define COARSE_HORIZONTAL_POSITION

/*
 * PRINT_AS_MUCH_AS_POSSIBLE_ON_LOW_MEMORY (default on)
 * 
 * In case of full-page graphics with heavy dithering,
 * the printer memory runs low in the middle of printing
 * a page. No amount of polling will help. So there are 
 * 3 options:
 * (1) polling forever and wait for a miracle.
 * (2) abort or push more data forward, let 
 *     the printer crashes and spit out a blank page 
 *     (at least in the case of 5700L)
 * (3) Print all white for the bottom part of the page
 *      (i.e. print as much as possible)
 *
 * Choice (3) wastes paper/toner, but at least the user
 * get to see *something*, whereas (2) requires
 * power-recyling the printer. Both are bad...
 * 
 */
#define PRINT_AS_MUCH_AS_POSSIBLE_ON_LOW_MEMORY


/**
 *************************************************************
 * Correlations between some subsidiary/internal options.
 * Do not modify below this point unless you are familiar
 * with how this piece of software functions.
 *************************************************************
**/

#ifdef __unix__
#undef STRIPE_OVERFLOW_WORKAROUND_HEADER
#undef STRIPE_OVERFLOW_WORKAROUND_STRIPE_PAD
#define USE_DELTA_STRATEGY 
#endif

#ifdef EPL_DEBUG
#define VERBOSE
#endif


#ifdef HAVE_LIBUSB
#include "libusb/usb.h"
#endif 

#ifdef HAVE_LIBIEEE1284
#include "libieee1284/include/ieee1284.h"
#endif 

#if defined(HAVE_LIBUSB) || defined(HAVE_KERNEL_USB_DEVICE) || defined(HAVE_LIBIEEE1284) || defined(HAVE_KERNEL_1284) || defined(HAVE_NULLTRANS)
#define USE_FLOW_CONTROL
#endif

/* This 1024 comes from the kernel header */
#define MAX_DEVICE_ID_SIZE 1024

/* Use for the bidirectional routines for read size */
#define MAX_READ_SIZE 255
