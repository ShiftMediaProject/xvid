/**************************************************************************
 *
 *      XVID MPEG-4 VIDEO CODEC - Example for encoding and decoding
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/************************************************************************
 *		                    
 *  Test routine for XviD using the XviD-API                      
 *  (C) Christoph Lampert, 2002/01/19	              
 *		                    
 *  A sequence of YUV pics in PGM file format is encoded and decoded                     
 *		                   
 *  The program is plain C and needs no libraries except for libxvidcore, 
 *  so with UN*X you simply compile by
 * 
 *  gcc -o xvid_enc_dec xvid_enc_dec.c -lxvidcore 
 *
 *  Run without parameters, then PGM input input is read from stdin.
 *
 *  PGM/xvid-output is saved, if corresponding flags are set 
 * 
 *  input data can be generated e.g. out of MPEG2 with mpeg2dec -o pgmpipe 
 *	
 ************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>

#include "xvid.h"		/* comes with XviD */

#define ARG_FRAMERATE 25
#define ARG_BITRATE 900

int XDIM=0;
int YDIM=0;	// will be set when reading first image
int filenr = 0;

int save_m4v_flag = 0;		// save MPEG4-bytestream?
int save_dec_flag = 0;		// save decompressed bytestream?
char filepath[256] = "./";	// path where to save output

void *enchandle = NULL;
void *dechandle = NULL;

/*********************************************************************/
/*	Routines for file input/output, nothing specific to XviD     */
/*********************************************************************/

int read_pgmheader(FILE* handle)
{	int bytes,xsize,ysize,depth;
	char dummy[2];
	
	bytes = fread(dummy,1,2,handle);

	if ( (bytes < 2) || (dummy[0] != 'P') || (dummy[1] != '5' ))
   		return 1;
	fscanf(handle,"%d %d %d",&xsize,&ysize,&depth); 
	if ( (xsize > 1440) || (ysize > 2880 ) || (depth != 255) )
	{
		fprintf(stderr,"%d %d %d\n",XDIM,YDIM,depth);
	   	return 2;
	}
	if ( (XDIM==0) || (YDIM==0) )
	{	XDIM=xsize;
		YDIM=ysize;
	}

	return 0;
}

int read_pgmdata(FILE* handle, unsigned char *image)
{	int i;
	char dummy;
   
	unsigned char* buff1_ptr2 = image + XDIM*YDIM;
	unsigned char* buff1_ptr3 = image + XDIM*YDIM + XDIM/2*YDIM/2; 

	fread(image,XDIM,YDIM,stdin);
 
   	for (i=0;i<YDIM/2;i++)
	{
		fread(buff1_ptr2,XDIM/2,1,stdin);          
		buff1_ptr2 += XDIM/2;
		fread(buff1_ptr3,XDIM/2,1,stdin);      
		buff1_ptr3 += XDIM/2;
	}
	fread(&dummy,1,1,handle);				// should be EOF
	return 0;
}


/*********************************************************************/
/* Routines for encoding: init encoder, frame step, release encoder  */
/*********************************************************************/

#define FRAMERATE_INCR 1001
int enc_init()
{
	int xerr;
	
	XVID_INIT_PARAM xinit;
	XVID_ENC_PARAM xparam;

	xinit.cpu_flags = 0;
	xvid_init(NULL, 0, &xinit, NULL);

	xparam.width = XDIM;
	xparam.height = YDIM;
	if ((ARG_FRAMERATE - (int)ARG_FRAMERATE) == 0)
	{
	        xparam.fincr = 1;
	        xparam.fbase = (int)ARG_FRAMERATE;
	}
	else
	{
	        xparam.fincr = FRAMERATE_INCR;
	        xparam.fbase = (int)(FRAMERATE_INCR * ARG_FRAMERATE);
	}
	xparam.bitrate = ARG_BITRATE*1000; 
	xparam.rc_buffersize = 2048000; // amount of data you have to buffer for continous
								    // playback in a streaming app (in bytes)
	xparam.min_quantizer = 2;		
	xparam.max_quantizer = 31;
	xparam.max_key_interval = (int)ARG_FRAMERATE*10;

	xerr = xvid_encore(NULL, XVID_ENC_CREATE, &xparam, NULL);
	enchandle=xparam.handle;
	
	return xerr;
}

int  enc_stop()
{	int xerr;

	xerr = xvid_encore(enchandle, XVID_ENC_DESTROY, NULL, NULL);
        return xerr;
}

int  enc_main(unsigned char* image, unsigned char* bitstream, int *streamlength)
{	int xerr;

	XVID_ENC_FRAME xframe;
	XVID_ENC_STATS xstats;

	// general features
	xframe.general = XVID_H263QUANT; // we use h.263 quantisation
//	xframe.general = XVID_MPEGQUANT; // MPEG quantization

	xframe.general |= XVID_HALFPEL; // halfpel precision
	xframe.general |= XVID_INTER4V; // four motion vector mode

	// motion estimation (pmvfast) settings
	xframe.motion = PMV_HALFPELREFINE16 | PMV_EARLYSTOP16 |
					PMV_HALFPELDIAMOND8 | PMV_EARLYSTOP8;

	xframe.bitstream = bitstream;
	xframe.length = -1; 	// this is written by the routine

	xframe.image = image;
    xframe.colorspace = XVID_CSP_YV12;	// defined in <xvid.h>

    xframe.intra = -1; // let the codec decide between I-frame (1) and P-frame (0)
	xframe.quant = 0;
//        xframe.quant = QUANTI;	// is quant != 0, use a fixed quant (and ignore bitrate)


	xerr = xvid_encore(enchandle, XVID_ENC_ENCODE, &xframe, &xstats);

/*	        enc_result->is_key_frame = xframe.intra;
	        enc_result->quantizer = xframe.quant;
	        enc_result->total_bits = xframe.length * 8;
	        enc_result->motion_bits = xstats.hlength * 8;
	        enc_result->texture_bits = enc_result->total_bits - enc_result->motion_bits;
*/ 

/*  This is statictical data, e.g. for 2-pass. 
    If you are not interested in any of this, you can use NULL instead of &xstats
*/

	*streamlength = xframe.length;

	return xerr;
}


/*********************************************************************/
/* Routines for decoding: init encoder, frame step, release encoder  */
/*********************************************************************/

int dec_init()
{
	int xerr;
	
	XVID_INIT_PARAM xinit;
	XVID_DEC_PARAM xparam;

	xinit.cpu_flags = 0;
	xvid_init(NULL, 0, &xinit, NULL);
	xparam.width = XDIM;
	xparam.height = YDIM;
	
	xerr = xvid_decore(NULL, XVID_DEC_CREATE, &xparam, NULL);
	dechandle = xparam.handle;

	return xerr;
} 

int dec_main(unsigned char *m4v_buffer, unsigned char *rgb_buffer, int m4v_size)
{	
	int xerr;
	
	XVID_DEC_FRAME xframe;
	
        xframe.bitstream = m4v_buffer;
        xframe.length = m4v_size;
        xframe.image = rgb_buffer;
        xframe.stride = XDIM;
	xframe.colorspace = XVID_CSP_RGB24;		// XVID_CSP_USER is fastest (no memcopy involved)

/*       
	xframe.colorspace = XVID_CSP_NULL;  // use this if you want to skip a frame (no output)
*/

        xerr = xvid_decore(dechandle, XVID_DEC_DECODE, &xframe, NULL);

	return xerr;
}

int dec_stop()
{
	int xerr;
	xerr = xvid_decore(dechandle, XVID_DEC_DESTROY, NULL, NULL);

	return xerr;
}


/*********************************************************************/
/*                          Main program                             */
/*********************************************************************/

int main()
{
  unsigned char *divx_buffer = NULL;
  unsigned char *yuv_buffer = NULL;
  unsigned char *rgb_buffer = NULL;

  int status;
  int frame_size;
  int m4v_size;
  
  char filename[256];
  
  FILE *filehandle;

/* read YUV in pgm format from stdin */
  if (read_pgmheader(stdin))
    {
      printf("Wrong input format, I want YUV encapsulated in PGM\n");
      return 0;
    }

/* now we know the sizes, so allocate memory */

  yuv_buffer = (unsigned char *) malloc(XDIM*YDIM);	
  if (!yuv_buffer)
    goto free_all_memory;	// goto is one of the most underestimated instructions in C !!!

  divx_buffer = (unsigned char *) malloc(XDIM*YDIM*2);	// this should really be enough!
  if (!divx_buffer)
    goto free_all_memory;	// actually, every program should contain at least one goto 
    
  YDIM = YDIM*2/3; // PGM is YUV 4:2:0 format, so real image height is *2/3 of PGM picture 
    
  rgb_buffer = (unsigned char *) malloc(XDIM*YDIM*4);	
  if (!rgb_buffer)
    goto free_all_memory;	// but the more, the better!

/*********************************************************************/
/*	                   DIVX PART  Start                          */
/*********************************************************************/

  status = enc_init();
//       if (status)    
	  printf("Encore INIT return %d, handle=%lx\n", status, enchandle);

  status = dec_init();
//      if (status) 
	  printf("Decore INIT return %d\n", status);


/*********************************************************************/
/*	                         Main loop                           */
/*********************************************************************/

  do
    {
      status = read_pgmdata(stdin, yuv_buffer);	// read PGM data (YUV-format)
      if (status)
	{
	  fprintf(stderr, "Couldn't read PGM body: %d\n", status);	/* this should not happen */
	  continue;
	}

/*********************************************************************/
/*               encode and decode this frame                        */
/*********************************************************************/

	status = enc_main(yuv_buffer, divx_buffer, &m4v_size);

	printf("Frame %5d: encore-ENCODE status %d, m4v-stream length=%7d bytes\n",
		 filenr, status, m4v_size);

      if (save_m4v_flag)
	{
	  sprintf(filename, "%sframe%05d.m4v", filepath, filenr);
	  filehandle = fopen(filename, "wb");
	  fwrite(divx_buffer, m4v_size, 1, filehandle);
	  fclose(filehandle);
	}

      status = dec_main(divx_buffer, rgb_buffer, m4v_size);
	if (status)
		printf("Frame %5d: decore status %d\n",status);
      
      if (save_dec_flag)
	{
		sprintf(filename, "%sdec%05d.ppm", filepath, filenr);
 	       	filehandle=fopen(filename,"wb");
		if (filehandle)
	        {
                	fprintf(filehandle,"P6\n");		// rgb24 in PPM format
	      		fprintf(filehandle,"%d %d 255\n",XDIM,YDIM);
      			fwrite(rgb_buffer,XDIM,YDIM*3,filehandle);
                	fclose(filehandle);
		}
 	}

      filenr++;
      status = read_pgmheader(stdin);		// if it was the last PGM, stop after it
      if (status)
	{
	  fprintf(stderr, "Couldn't read next PGM header: %d\n", status); /* normally, just end of file */
	}
    }
  while (!status);

/*********************************************************************/
/*	   DIVX PART  Stop                        */
/*********************************************************************/

  	dec_stop();
//       if (status)    
	printf("Encore RELEASE return %d\n", status);

  	enc_stop();
//       if (status)    
	printf("Decore RELEASE return %d\n", status);

free_all_memory:

  if (rgb_buffer)
    free(rgb_buffer);
  if (divx_buffer)
    free(divx_buffer);
  if (yuv_buffer)
    free(yuv_buffer);

  return 0;
}
