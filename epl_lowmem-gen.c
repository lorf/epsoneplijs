#include <stdio.h>
#include "low_mem_msg.xbm"
#include "epl_compress.h"

void stream_append(typ_stream *stream, int d, int l);

int main(void)
{
  int i, j;
  char *ptr_row_prev;
  char *ptr_row_current;
  char *ptr_temp;
  extern struct proto op_table[];
  extern struct proto run_table[];
  int bytes_per_row_padded = low_mem_msg_width;
  typ_stream *stream = (typ_stream *)malloc(sizeof(typ_stream));
  
  stream->start = (char *)malloc(bytes_per_row_padded * 64 * 5 / 4 + 2 + 1);
  ptr_row_prev = (char *)malloc(bytes_per_row_padded);
  ptr_row_current = (char *)malloc(bytes_per_row_padded);
  
  stream_init(stream);

  stream_append(stream, 0x1d,13);
  stream_append(stream, 0x1d,13);
  stream_append(stream, 0x1d,13);
  stream_append(stream, 0x1d,13);

  for (i = 0 ; i < bytes_per_row_padded ; i++)
    {
      *(ptr_row_prev + i) = (char) 0xff;
    }
  for (j = 0 ; j < low_mem_msg_height ; j++)
    {
      for (i = 0 ; i < bytes_per_row_padded ; i++)
	{
	  *(ptr_row_current + i) = (char) 0xff;
	}
      for (i = 0 ; i < low_mem_msg_width ; i++)
	{
	  if (low_mem_msg_bits[(j * low_mem_msg_width + i)/8] & (0x01 << (i % 8)))
	    {
	    *(ptr_row_current +i) = 0x00;
	    }
	  else 
	    {
	      *(ptr_row_current +i) = 0xff;
	    }
	  fprintf(stderr, "%s", ((*(ptr_row_current +i))? ".":"#"));
	}
      epl_compress_row(stream, 
		       ptr_row_current, 
		       ptr_row_prev, bytes_per_row_padded);
      stream_append(stream, 0x1d,13);
      stream_append(stream, 0x1d,13);
      stream_append(stream, 0x1d,13);
      if (j % 4)
	{
	  stream_append(stream, 0x1d,13);
	  stream_append(stream, 0x1d,13);
	  stream_append(stream, 0x1d,13);
	  stream_append(stream, 0x1d,13);
	}
      ptr_temp = ptr_row_current;
      ptr_row_current = ptr_row_prev;
      ptr_row_prev = ptr_temp;
      fprintf(stderr, "size = %i\n", stream->count);
    }
  stream_pad16bit(stream);

  fprintf(stderr, "size = %i\n", stream->count);
  for(i = 0; i < stream->count;  i++)
    {
      fprintf(stderr, "0x%2.2X, ", (0xff & *(stream->start + i)));
      if (!((i+1) % 12))
	{
	  fprintf(stderr, "\n");
	}
    }
  exit(0);
}
