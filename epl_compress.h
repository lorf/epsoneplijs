/**
 * Copyright (c) 2003 Hin-Tak Leung
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

/* 
   The main program passes the stream structure in 
   for every row fetched. 
   Therefore it should only have access to 

   compress row, stream_pad16 

   The rest are internal to maintaining
   the compressed stream and don't need to be seen
   by the main program.
*/

/*
   Damn! It is logical to do it in an 
   object-oriented manner...
*/

typedef struct _Stream {
  char *start;       /* storage for compressed stream */
  char *ptr;         /* first empty location */
  int count;         /* number of byte stored */
  unsigned int temp; /* storage for temp bits */
  int bits;          /* count in temp */
} typ_stream;

void stream_init(typ_stream *stream);

void stream_pad16bit(typ_stream *stream);

long epl_compress_row(typ_stream *stream, char *ptr_current, 
			 char *ptr_row_prev, int wbytes);
