/**
 * Copyright (c) 2003 Hin-Tak Leung, Roberto Ragusa
 *
**/

/**
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
**/

#define USE_4BIT_CODE

#include "epl_config.h"
#include "epl_compress.h"
#include "epl_lowmem_msg.h"

struct proto {
    int code;    /* bit-code */
    int width;     /* width of bit-code  */
};

/* run count table */
/* 
   putting the termination code at zero is for convenience, 
   not correct 
*/

#define THE_REST 0
#define CONTINUE 8

static struct proto run_table[] = 
  {
    { 0x0007, 11 }, /* the rest - 0000000 0111 */
    { 0x00, 1 },    /* count 1  -            0 */
    { 0x01, 2 },    /* count 2  -           01 */   
    { 0x03, 4 },    /* count 3  -         0011 */ 
    { 0x0b, 4 },    /* count 4  -         1011 */
    { 0x0f, 5 },    /* count 5  -        01111 */
    { 0x1f, 6 },    /* count 6  -       011111 */
    { 0x3f, 6 },    /* count 7  -       111111 */
    { 0x07, 4 }     /* count>7  -         0111 */
  };

/* op code table */

#define CACHED     0
#define LITERAL    1
#define COPY_P     2
#define COPY_L1    3
#define COPY_L2    4
#define COPY_L3    5

static struct proto op_table[] = 
  {
    {0x00, 2}, /* Cached             -   00 */
    {0x02, 2}, /* Literal            -   10 */
    {0x01, 2}, /* Copy row prev      -   01 */
    {0x03, 3}, /* Copy 1st byte left -  011 */
    {0x07, 4}, /* Copy 2nd byte left - 0111 */
    {0x0f, 4}, /* Copy 3rd byte left - 1111 */
  };


#ifdef USE_4BIT_CODE
/* the actual value of CACHE_INVALID doesn't matter, as long as it is not 0x00 to 0x0f */
#define CACHE_INVALID 0xFF
static unsigned char cache[16];
static unsigned char byte2cache[256];
static int lri;

void cache_init(void)
{
  int i;
  for( i = 0 ; i < 16 ; i++)
    {
      cache[i] = i;
      byte2cache[i] = i;
    }
  for(i = 16 ; i < 256 ; i++)
    {
      byte2cache[i] = CACHE_INVALID;
    }
  lri = 0;
}
#endif

void stream_append(typ_stream *stream, int d, int l); 
void stream_pad16bit(typ_stream *stream); 
int run_length_find(typ_stream *stream, char *new,char *old, int remain) ;

void stream_init(typ_stream *stream)
{	  
  stream->ptr = stream->start;
  stream->count = 0 ;
  stream->temp = 0;
  stream->bits = 0;
#ifdef USE_4BIT_CODE
  cache_init();  /* doing it here is a ugly hack (but correct) */
#endif
}

void stream_append(typ_stream *stream, int d, int l) 
{
  stream->temp |= d << stream->bits;
  stream->bits +=  l ;

  if ( stream->bits >= 16 ) 
    {
      *(stream->ptr) = (stream->temp >> 8 ) & 0xff;
      (stream->ptr)++;
      *(stream->ptr) = stream->temp & 0xff;
      (stream->ptr)++;

      stream->temp >>= 16;
      stream->bits -= 16;
      stream->count += 2;
    }
}

void stream_pad16bit(typ_stream *stream)
{
  if ( stream->bits > 0 ) 
    {
      stream_append(stream, 0, 16 - stream->bits);
    }
}

int run_length_find(typ_stream *stream, 
		    char *new, char *old, 
		    int remain) 
{
  int length = 0;
  int actual_count;

  /* if it gets to this section, we know already length > 0,
     but it won't hurt to do it again */

  while (length < remain) 
    {
      if ( *new == *old ) 
	{
	  old++;
	  new++;
	  length++;
	}
      else 
	{
	  break;
	}
    }

  actual_count = length ;
  
  if ( length == remain )
    {
      length = THE_REST;
    }
  
  if(length < 8) /* caution! this depends on THE_REST = 0 */
    {
      stream_append(stream, run_table[length].code, run_table[length].width);
    } 
  else 
    {
      stream_append(stream, run_table[CONTINUE].code, run_table[CONTINUE].width);
      
      while(length >= 127)
	{
	  stream_append(stream, 127, 7);
	  length -= 127;
	}
      stream_append(stream, length, 7);
    }
  return actual_count;
}



int epl_compress_row(typ_stream *stream, 
			 char *ptr_row_current, 
			 char *ptr_row_prev, int wbytes)
{
  int x = 0 ;

    while ( x < wbytes ) {
#ifdef USE_DELTA_STRATEGY
      if ( *(ptr_row_current + x) 
           == *(ptr_row_prev + x)) 
        {
          
          stream_append(stream,op_table[COPY_P].code,
			op_table[COPY_P].width);
          x += run_length_find(stream, ptr_row_current + x,
			       ptr_row_prev + x, 
			       wbytes - x);
        }
#else
      if (0)
        {
	}
#endif
      else if ((x >= 1) 
	       && ( *(ptr_row_current + x) 
		    == *(ptr_row_current + x - 1)))
        {
          stream_append(stream,op_table[COPY_L1].code, 
		        op_table[COPY_L1].width);
          x += run_length_find(stream, ptr_row_current + x,
			       ptr_row_current + x - 1,
			       wbytes - x);
        }
      else if((x >= 2)
	      && ( *(ptr_row_current + x) 
	          == *(ptr_row_current + x - 2)))
        {
          stream_append(stream,op_table[COPY_L2].code, 
		        op_table[COPY_L2].width);
          x += run_length_find(stream, ptr_row_current + x,
			       ptr_row_current + x - 2,
			       wbytes - x);
        }
      else if((x >= 3) 
	      && ( *(ptr_row_current + x) 
	           == *(ptr_row_current + x - 3)))
        {
          stream_append(stream,op_table[COPY_L3].code, 
		        op_table[COPY_L3].width);
          x += run_length_find(stream, ptr_row_current + x,
			       ptr_row_current + x - 3,
			       wbytes - x);
        }
      else
        {
          /* The printer takes 1 as black, 
             ghostscript raster generates max value for white (i.e. 1 for B/W),
             hence literal requires bit-reversing.
          */ 
          int byte = (*(ptr_row_current + x) ^ 0xff) & 0xff;
#ifdef USE_4BIT_CODE
          if (byte2cache[byte] != CACHE_INVALID)
            { 
              stream_append(stream,
	                    op_table[CACHED].code, 
		            op_table[CACHED].width);
              stream_append(stream,
	                    byte2cache[byte], 
                            4);
              x++;
            }
          else
#endif
            { 
              stream_append(stream,op_table[LITERAL].code, 
		        op_table[LITERAL].width);
              stream_append(stream, byte, 8);
#ifdef USE_4BIT_CODE
              byte2cache[cache[lri]] = CACHE_INVALID;
	      cache[lri] =  byte;
	      byte2cache[byte] = lri;
              lri = (lri + 1) & 0xf;
#endif
              x++;
            }
        }
    }
  return stream->count ;
}


/* hard-coded blank stripe put here for efficiency */
static unsigned char blank_stripe[] = {
  0xa0, 0x1d, 0x74, 0x03, 0x0e, 0x80, 0x01, 0xd0, 0x40, 0x3a, 0xe8, 0x07, 0x1d, 0x00, 0x03, 0xa0, 
  0x80, 0x74, 0xd0, 0x0e, 0x3a, 0x01, 0x07, 0x40, 0x00, 0xe8,
  0xa0, 0x1d, 0x74, 0x03, 0x0e, 0x80, 0x01, 0xd0, 0x40, 0x3a, 0xe8, 0x07, 0x1d, 0x00, 0x03, 0xa0, 
  0x80, 0x74, 0xd0, 0x0e, 0x3a, 0x01, 0x07, 0x40, 0x00, 0xe8,
  0xa0, 0x1d, 0x74, 0x03, 0x0e, 0x80, 0x01, 0xd0, 0x40, 0x3a, 0xe8, 0x07, 0x1d, 0x00, 0x03, 0xa0, 
  0x80, 0x74, 0xd0, 0x0e, 0x3a, 0x01, 0x07, 0x40, 0x00, 0xe8,
  0xa0, 0x1d, 0x74, 0x03, 0x0e, 0x80, 0x01, 0xd0, 0x40, 0x3a, 0xe8, 0x07, 0x1d, 0x00, 0x03, 0xa0, 
  0x80, 0x74, 0xd0, 0x0e, 0x3a, 0x01, 0x07, 0x40, 0x00, 0xe8,
};


void make_blank(typ_stream *stream)
{
  int idx = 0; 
  stream->count = 104;
  for (idx = 0 ; idx < 104; idx++)
    {
      *(stream->start + idx) = blank_stripe[idx];
    }
  return;
}

void make_lowmem_msg(typ_stream *stream)
{
  int idx = 0; 
  stream->count = LOWMEM_MSG_SIZE;
  for (idx = 0 ; idx < stream->count; idx++)
    {
      *(stream->start + idx) = lowmem_msg[idx];
    }
  return;
}
