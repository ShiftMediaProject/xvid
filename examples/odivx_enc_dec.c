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
 *  Test routine for XviD using the OpenDivX/Divx4-API                      
 *  (C) Christoph Lampert, 2002/01/19	              
 *		                    
 *  A sequence of YUV pics in PGM file format is encoded and decoded                     
 *		                   
 *  The program is plain C and needs no libraries except for libxvidcore, 
 *  so with UN*X you simply compile by
 * 
 *  gcc -o odivx_enc_dec odivx_enc_dec.c -lxvidcore 
 *
 *  Run without parameters, then PGM input input is read from stdin.
 *
 *  PGM/xvid-output is saved, if corresponding flags are set 
 * 
 *  input data can be generated e.g. out of MPEG2 with mpeg2dec -o pgmpipe 
 *	
 ************************************************************************/

#include <stdio.h>
#include <malloc.h>

#include "encore2.h"
#include "decore.h"		/* these come with XviD */

#define ARG_FRAMERATE 25
#define ARG_BITRATE 900

int QUALITY =5;
int QUANTI = 0;		// used for fixed-quantizer encoding 

int XDIM=0;
int YDIM=0;	// will be set when reading first image
int filenr = 0;

int save_m4v_flag = 0;		// save MPEG4-bytestream?
int save_dec_flag = 0;		// save decompressed bytestream?
char filepath[256] = ".";	// path where to save 

void *enchandle = NULL;		// enchandle is a void*, written by encore
const long dechandle = 0x0815;	// dechandle is a unique constant!!! 

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
	int status;
	ENC_PARAM enc_param;

	enc_param.x_dim = XDIM;
	enc_param.y_dim = YDIM;
	enc_param.framerate = ARG_FRAMERATE;		// a float
	enc_param.bitrate = ARG_BITRATE*1000;

	enc_param.rc_period = 2000;
	enc_param.rc_reaction_period = 10;
	enc_param.rc_reaction_ratio = 20;

	enc_param.max_quantizer = 31;
	enc_param.min_quantizer = 1;

	enc_param.quality = QUALITY;

	enc_param.use_bidirect = 0;	// use bidirectional coding
	enc_param.deinterlace = 0;	// fast deinterlace
	enc_param.obmc = 0;		// flag to enable overlapped block motion compensation mode

	enc_param.max_key_interval = (int)ARG_FRAMERATE*10;
	enc_param.handle = NULL;	//will be filled by encore

	status = encore(enchandle, ENC_OPT_INIT, &enc_param, NULL);
	enchandle = enc_param.handle;

//	if (status)    
		printf("Encore INIT return %d, handle=%lx\n", status, enchandle);

	return status;
}

int  enc_stop()
{	int status;

	status = encore(enchandle, ENC_OPT_RELEASE, NULL, NULL);	
//   	if (status) 
		printf("Encore RELEASE return %d\n", status);

        return status;
}

int  enc_main(unsigned char* image, unsigned char* bitstream, int *streamlength)
{	int status;

	ENC_FRAME enc_frame;
	ENC_RESULT enc_result;
	
	enc_frame.image = (void *) image; 
	enc_frame.bitstream = (void *) bitstream;
	enc_frame.length = 0;			// filled by encore
	enc_frame.colorspace = ENC_CSP_YV12;	// input is YUV
	enc_frame.mvs = NULL;		// unsupported

	if (QUANTI==0)
	{
		status = encore(enchandle, ENC_OPT_ENCODE, &enc_frame, &enc_result);
	}
	else
	{	enc_frame.quant = QUANTI;
		enc_frame.intra = -1;		// let encoder decide if frame is INTER/INTRA
		status = encore(enchandle, ENC_OPT_ENCODE_VBR, &enc_frame, &enc_result);
	}

/* ENC_OPT_ENCODE is used to encode using ratecontrol alg. and bitrate from enc_init, 
   ENC_OPT_ENCODE_VBR is used to encode with fixed quantizer and INTER/INTRA mode 
*/

	*streamlength = enc_frame.length;

	return status;
}


/*********************************************************************/
/* Routines for decoding: init encoder, frame step, release encoder  */
/*********************************************************************/

int dec_init()
{
	int status;
	
	DEC_PARAM dec_param;
	DEC_SET dec_set;
	
	dec_param.x_dim = XDIM;
	dec_param.y_dim = YDIM;
	dec_param.output_format = DEC_RGB24;	//   output color format, , see <decore.h>
	
	dec_param.time_incr = 20;

	status = decore(dechandle, DEC_OPT_INIT, &dec_param, NULL);
//      if (status) 
		printf("Decore INIT return %d\n", status);

// We don't do any postprocessing here... 

/*	dec_set.postproc_level = 0;
	status = decore(dechandle, DEC_OPT_SETPP, &dec_set, NULL);
	if (status) 
		printf("Decore POSTPROC %d return %d\n", dec_set.postproc_level, status);
*/

	return status;
} 

int dec_main(unsigned char *m4v_buffer, unsigned char *rgb_buffer, int m4v_size)
{	
	int status;
	
	static DEC_FRAME dec_frame;
	static DEC_FRAME_INFO dec_frame_info;

	dec_frame.length = m4v_size;
	dec_frame.bitstream = m4v_buffer;
	dec_frame.bmp = rgb_buffer;
	dec_frame.render_flag = 1;		// 0 means: skip this frame
	dec_frame.stride = XDIM;

	status = decore(dechandle, DEC_OPT_FRAME, &dec_frame, &dec_frame_info);
	if (status)
		printf("Decore Frame status %d!\n", status);

	return status;
}

int dec_stop()
{
	int status;
	status = decore(dechandle, DEC_OPT_RELEASE, NULL, NULL);
//	if (status) 
		printf("Decore RELEASE return %d\n", status);
	return status;
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
    goto free_all_memory;	// actually, every program should contain a goto 
    
  YDIM = YDIM*2/3; // PGM is YUV 4:2:0 format, so height is *3/2 too much
    
  rgb_buffer = (unsigned char *) malloc(XDIM*YDIM*4);	
  if (!rgb_buffer)
    goto free_all_memory;	// the more, the better!

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
	  fprintf(stderr, "PGM-Data-Error: %d\n", status);	/* this should not happen */
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
	  sprintf(filename, "%s/frame%05d.m4v", filepath, filenr);
	  filehandle = fopen(filename, "wb");
	  fwrite(divx_buffer, m4v_size, 1, filehandle);
	  fclose(filehandle);
	}

      status = dec_main(divx_buffer, rgb_buffer, m4v_size);
	if (status)
		printf("Frame %5d: decore status %d\n",status);
      
      if (save_dec_flag)
	{
		sprintf(filename, "%s/dec%05d.ppm", filepath, filenr);
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
	  fprintf(stderr, "PGM-Header-Error: %d\n", status); /* normally, just end of file */
	}
    }
  while (!status);


/*********************************************************************/
/*	   DIVX PART  Stop                        */
/*********************************************************************/


/* Stop XviD */

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
