#include <stdio.h>

int main(void){
  int hstripes;

#define  wpix 4768
#define  hpix 6912

  /*   wbytes=wpix/8 ; 6796 = 849.5 - hpix needs to be divisible by 64*/
#define  wbytes 596

/* wsizeout = wbyte * 80 +2 */
#define wsizeout 47682

  /* wsizein = wbytes * 64 */
#define wsizein 38144

  unsigned char inbuffer[wsizein];
  unsigned char whiterow[wbytes];
  unsigned char *thisrow, *rowabove;
  int i;
  int x,y,s;
  int slen;

/* STREAM HANDLING */

  unsigned char stream_outbuffer[wsizeout];/* allow 50% expansion for safety */
  unsigned char *stream_ptr;
  int stream_temp,stream_bits;

#define stream_begin() do{\
    stream_ptr=&stream_outbuffer[0];\
    stream_temp=0;\
    stream_bits=0;\
  } while(0)

#define stream_emit(d,l) do{\
    stream_emit_nocheck((d),(l));\
    stream_check();\
  } while(0)

#define stream_emit_nocheck(d,l) do{\
    stream_temp|=(d)<<stream_bits;\
    stream_bits+=(l);\
  } while(0)

#define stream_check() do{\
    if(stream_bits>=16){\
      *(stream_ptr++)=(stream_temp>>8)&0xff;\
      *(stream_ptr++)=stream_temp&0xff;\
      stream_temp>>=16;\
      stream_bits-=16;\
    }\
  } while(0)

#define stream_end() do{\
    stream_emit(0,(16-stream_bits)%16);\
  } while(0)


/* RUN LENGTH */

  int run_length;
  int run_length_table_d[]={/**/0,/*0*/0,/*01*/1,/*0011*/3,/*1011*/0xb,/*01111*/0xf,/*011111*/0x1f,/*111111*/0x3f};
  int run_length_table_l[]={    0,     1,      2,        4,          4,           5,             6,             6};
  int run_length_returned;

#define run_length_find(b1,b2,maxl) do{\
    for(run_length=1;run_length<(maxl)&&(b1)[run_length]==(b2)[run_length];run_length++);\
    run_length_returned=run_length;\
    if(run_length==(maxl)){\
      stream_emit(/*0000000 0111*/0x007,4+7);\
    }\
    else if(run_length>=8){\
      stream_emit(/*0111*/0x7,4);\
      while(run_length>=127){\
        stream_emit(127,7);\
        run_length-=127;\
      }\
      stream_emit(run_length,7);\
    }\
    else{\
      stream_emit(run_length_table_d[run_length],run_length_table_l[run_length]);\
    }\
  } while(0)


/* MISC DATA */

unsigned char header_data[]={
  0x1b,0x01,0x40,0x45,0x4a,0x4c,0x20,0x0a, 0x40,0x45,0x4a,0x4c,0x20,0x53,0x54,0x41,  /* |..@EJL .@EJL STA| */
  0x52,0x54,0x4a,0x4f,0x42,                                                          /* |RTJOB MACHINE="e| */
                                                                                     /* |xperiment" USER=| */
                                                                                     /* |"epl5x00l_driver| */
       0x0a,0x40,0x45,0x4a,0x4c,0x20,0x45, 0x4e,0x20,0x4c,0x41,0x3d,0x45,0x53,0x43,  /* |".@EJL EN LA=ESC| */
  0x2f,0x50,0x41,0x47,0x45,0x0a,0x1d,0x34, 0x65,0x70,0x73,0x7b,0x49,0x00,0x00,0x00,  /* |/PAGE..4eps{I...| */
  0x00,0x1d,0x39,0x65,0x70,0x73,0x7b,0x49, 0x02,0x00,0x01,0x00,0x01,0x00,0x00,0x03,  /* |..9eps{I........| */
  0x00,0x1d,0x32,0x36,0x65,0x70,0x73,0x7b, 0x49,0x04,0x00,0x0e,0x40,0x02,0x54,0x00,  /* |..26eps{I...@.T.| */
  0x00,0x00,0x00,0x1a,0x8c,0x12,0x84,0x00, 0x6b,0x00,0x00,0x01,0xff,0xfe,0x00,0x00,  /* |........k.......| */
  0x00,0x00,0x01                                                                     /* |...|              */
};
unsigned char footer_data[]={
  0x1d,0x32,0x65,0x70,0x73,0x7b,0x49,0x05, 0x00,0x1d,0x32,0x65,0x70,0x73,0x7b,0x49,  /* |.2eps{I...2eps{I| */
  0x03,0x00,0x1d,0x32,0x65,0x70,0x73,0x7b, 0x49,0x01,0x00,0x1b,0x01,0x40,0x45,0x4a,  /* |...2eps{I....@EJ| */
  0x4c,0x20,0x0a                                                                     /* |L .@EJL EJ ...@E| */
                                                                                     /* |JL .|             */
};

  hstripes=(hpix+63)/64; /* +63 is a ugly hack, we don't know what we're printing */

    

  for(i=0;i<wbytes;i++) whiterow[i]=0;

  fread(inbuffer,1,68,stdin);			/* skip 13 bytes (horror!)*/
  fwrite(header_data,1,sizeof(header_data),stdout);

  for(s=0;s<hstripes;s++){
    fread(inbuffer,1,wbytes*64,stdin);
    rowabove=&whiterow[0];			/* special case */
    thisrow=&inbuffer[0];
    stream_begin();
    for(y=0;y<64;y++){
      for(x=0;x<wbytes;){
        if(thisrow[x]==rowabove[x]){
          stream_emit(/*01*/0x1,2);
          run_length_find(&thisrow[x],&rowabove[x],wbytes-x);
          x+=run_length_returned;
        }
        else if(x-1>=0&&thisrow[x]==thisrow[x-1]){
          stream_emit(/*011*/0x3,3);
          run_length_find(&thisrow[x],&thisrow[x-1],wbytes-x);
          x+=run_length_returned;
  	}
        else if(x-2>=0&&thisrow[x]==thisrow[x-2]){
          stream_emit(/*0111*/0x7,4);
          run_length_find(&thisrow[x],&thisrow[x-2],wbytes-x);
          x+=run_length_returned;
  	}
        else if(x-3>=0&&thisrow[x]==thisrow[x-3]){
          stream_emit(/*1111*/0xf,4);
          run_length_find(&thisrow[x],&thisrow[x-3],wbytes-x);
          x+=run_length_returned;
  	}
        else if(0/*because_it_is_not_reliable*/){					/* currently not enabled */
          stream_emit(/*00*/0x0,2);
  	  //stream_emit(i_dont_know_exactly_what,4);
          x++;
  	}
        else{
          stream_emit(/*10*/0x2,2);
  	  stream_emit(thisrow[x],8);
          x++;
  	}
      }
      rowabove=thisrow;
      thisrow+=wbytes;
    }
    stream_end();
    slen=stream_ptr-stream_outbuffer;
    fprintf(stdout,"\x1d%deps{I%c%c%c%c%c%c%c",slen+7,6,0,1,(slen>>24)&0xff,(slen>>16)&0xff,(slen>>8)&0xff,slen&0xff);
    fwrite(stream_outbuffer,1,slen,stdout);
  }

  fwrite(footer_data,1,sizeof(footer_data),stdout);
  return 0;
}
