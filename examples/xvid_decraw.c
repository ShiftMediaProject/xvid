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
 *  Test routine for XviD decoding                       
 *  (C) Christoph Lampert, 2002/08/17
 *		                    
 *  An MPEG-4 bitstream is read from stdin and decoded,
 *  the speed for this is measured 
 *		                   
 *  The program is plain C and needs no libraries except for libxvidcore, 
 *  and maths-lib, so with UN*X you simply compile by
 * 
 *   gcc xvid_decraw.c -lxvidcore -lm -o xvid_decraw
 *
 *  You have to specify the image dimensions (until the add the feature 
 *  to read this from the bitstream)
 * 
 *  Parameters are: xvid_stat XDIM YDIM 
 * 
 *  output and indivual m4v-files are saved, if corresponding flags are set 
 * 
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>		// needed for log10
#include <sys/time.h>		// only needed for gettimeofday

#include "../src/xvid.h"		/* comes with XviD */

#define ABS_MAXFRAMENR 9999               // max number of frames

int XDIM=720;
int YDIM=576;
int i,filenr = 0;

int save_dec_flag = 1;		// save decompressed bytestream?
int save_m4v_flag = 0;		// save bytestream itself?

char filepath[256] = "./";	// the path where to save output 

void *dec_handle = NULL;		// handle for decoding

/*********************************************************************/
/*	               "statistical" functions                     	     */
/*                                                                   */
/*  these are not needed for decoding itself, but for measuring      */
/*  time (and maybe later quality), there in nothing specific to     */
/*  XviD in these                                                    */
/*                                                                   */
/*********************************************************************/

double msecond()  	
/* return the current time in seconds(!)  */
{	
	struct timeval  tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec + tv.tv_usec * 1.0e-6;
}


/*********************************************************************/
/*	              input and output functions                         */
/*                                                                   */
/* the are small and simple routines for writing image               */
/* image. It's just for convenience, again nothing specific to XviD  */
/*                                                                   */
/*********************************************************************/

int write_pgm(char *filename, unsigned char *image)
{
	FILE *filehandle;
	filehandle=fopen(filename,"wb");
	if (filehandle)
        {
		fprintf(filehandle,"P5\n\n");		// 
		fprintf(filehandle,"%d %d 255\n",XDIM,YDIM*3/2);
		fwrite(image,XDIM,YDIM*3/2,filehandle);

		fclose(filehandle);
		return 0;
	}
	else
		return 1;
}


int write_ppm(char *filename, unsigned char *image)
{
	FILE *filehandle;
	filehandle=fopen(filename,"wb");
	if (filehandle)
        {
		fprintf(filehandle,"P6\n\n");		// 
		fprintf(filehandle,"%d %d 255\n",XDIM,YDIM);
		fwrite(image,XDIM,YDIM*3,filehandle);

		fclose(filehandle);
		return 0;
	}
	else
		return 1;
}

/*********************************************************************/
/* Routines for decoding: init encoder, frame step, release encoder  */
/*********************************************************************/

int dec_init(int use_assembler)	/* init decoder before first run */
{
        int xerr;

        XVID_INIT_PARAM xinit;
        XVID_DEC_PARAM xparam;

		if(use_assembler)
#ifdef ARCH_IA64
			xinit.cpu_flags = XVID_CPU_FORCE | XVID_CPU_IA64;
#else
			xinit.cpu_flags = 0;
#endif
		else
			xinit.cpu_flags = XVID_CPU_FORCE;

        xvid_init(NULL, 0, &xinit, NULL);
        xparam.width = XDIM;
        xparam.height = YDIM;

        xerr = xvid_decore(NULL, XVID_DEC_CREATE, &xparam, NULL);
        dec_handle = xparam.handle;

        return xerr;
}

int dec_main(unsigned char *m4v_buffer, unsigned char *out_buffer,int *m4v_size)
{	/* decode one frame  */

        int xerr;
        XVID_DEC_FRAME xframe;

        xframe.bitstream = m4v_buffer;
        xframe.length = 9999;
   	    xframe.image = out_buffer;
        xframe.stride = XDIM;
   	    xframe.colorspace = XVID_CSP_YV12;             

		do {
	        xerr = xvid_decore(dec_handle, XVID_DEC_DECODE, &xframe, NULL);

			*m4v_size = xframe.length;
	        xframe.bitstream += *m4v_size;

		} while (*m4v_size<7);	/* this skips N-VOPs */
		
        return xerr;
}

int dec_stop()	/* close decoder to release resources */
{
        int xerr;
        xerr = xvid_decore(dec_handle, XVID_DEC_DESTROY, NULL, NULL);

        return xerr;
}


/*********************************************************************/
/*                          Main program                             */
/*********************************************************************/

int main(int argc, char *argv[])
{
  unsigned char *divx_buffer = NULL;
  unsigned char *divx_ptr = NULL;
  unsigned char *out_buffer = NULL;

  double dectime;
  double totaldectime=0.;
  
  long totalsize=0;
  int status;
  
  int m4v_size;
  int frame_type[ABS_MAXFRAMENR];
  int use_assembler=1;
  
  char filename[256];
  
  FILE *filehandle;

	if (argc>=3)
	{	XDIM = atoi(argv[1]);
	  	YDIM = atoi(argv[2]);
		if ( (XDIM <= 0) || (XDIM >= 2048) || (YDIM <=0) || (YDIM >= 2048) )
		{	fprintf(stderr,"Wrong frames size %d %d, trying PGM \n",XDIM, YDIM); 
		}
	}
	if (argc>=4 && !strcmp(argv[3],"noasm")) 
	  use_assembler = 0;
  

/* allocate memory */

  divx_buffer = (unsigned char *) malloc(10*XDIM*YDIM);	
  // this should really be enough memory!
  if (!divx_buffer)
    goto free_all_memory;	
  divx_ptr = divx_buffer+10*XDIM*YDIM;
    
  out_buffer = (unsigned char *) malloc(4*XDIM*YDIM);	/* YUV needs less */
  if (!out_buffer)
    goto free_all_memory;	


/*********************************************************************/
/*	                   XviD PART  Start                          */
/*********************************************************************/

	status = dec_init(use_assembler);
	if (status) 
	{
		printf("Decore INIT problem, return value %d\n", status);
		goto release_all;
	}


/*********************************************************************/
/*	                         Main loop                           */
/*********************************************************************/

  do
    {

	if (divx_ptr > divx_buffer+5*XDIM*YDIM)	/* buffer more than half empty */
	{	int rest=(divx_buffer+10*XDIM*YDIM-divx_ptr);
		if (rest)
			memcpy(divx_buffer, divx_ptr, rest);
		divx_ptr = divx_buffer; 
		fread(divx_buffer+rest, 1, 5*XDIM*YDIM, stdin);	/* read new data */
	}

	dectime = -msecond();
	status = dec_main(divx_ptr, out_buffer, &m4v_size);

	if (status)
	{
		break;
	}
	dectime += msecond();
	divx_ptr += m4v_size;

	totalsize += m4v_size;
      
	printf("Frame %5d: dectime =%6.1f ms length=%7d bytes \n", 
			filenr, dectime*1000, m4v_size);

	if (save_m4v_flag)
	{
		sprintf(filename, "%sframe%05d.m4v", filepath, filenr);
		filehandle = fopen(filename, "wb");
		fwrite(divx_buffer, m4v_size, 1, filehandle);
		fclose(filehandle);
	}
	totaldectime += dectime;      
      
      
/*********************************************************************/
/*        analyse the decoded frame and compare to original          */
/*********************************************************************/
      
	if (save_dec_flag)
	{
		sprintf(filename, "%sdec%05d.pgm", filepath, filenr);
 	       	write_pgm(filename,out_buffer);
 	}

	filenr++;

   } while ( (status>=0) && (filenr<ABS_MAXFRAMENR) );

      
/*********************************************************************/
/*     calculate totals and averages for output, print results       */
/*********************************************************************/

	totalsize    /= filenr;
	totaldectime /= filenr;
	
	fprintf(stderr,"Avg: dectime %5.2f ms, %5.2f fps, filesize =%d\n",
		1000*totaldectime, 1./totaldectime, totalsize);
		
/*********************************************************************/
/*	                   XviD PART  Stop                           */
/*********************************************************************/

release_all:

  	if (dec_handle)
	{
	  	status = dec_stop();
		if (status)    
			printf("Decore RELEASE problem return value %d\n", status);
	}

free_all_memory:
	free(out_buffer);
	free(divx_buffer);

  return 0;
}
