/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Console based decoding test application  -
 *
 *  Copyright(C) 2002-2003 Christoph Lampert
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: xvid_decraw.c,v 1.7 2003-02-16 05:11:39 suxen_drol Exp $
 *
 ****************************************************************************/

/*****************************************************************************
 *		                    
 *  Application notes :
 *		                    
 *  An MPEG-4 bitstream is read from an input file (or stdin) and decoded,
 *  the speed for this is measured.
 *		                   
 *  The program is plain C and needs no libraries except for libxvidcore, 
 *  and maths-lib, so with UN*X you simply compile by
 *
 *   gcc xvid_decraw.c -lxvidcore -lm -o xvid_decraw
 *
 *  You have to specify the image dimensions (until the add the feature 
 *  to read this from the bitstream)
 *
 * Usage : xvid_decraw <-w width> <-h height> [OPTIONS]
 * Options :
 *  -asm           : use assembly optimizations (default=disabled)
 *  -w integer     : frame width ([1.2048])
 *  -h integer     : frame height ([1.2048])
 *  -i string      : input filename (default=stdin)
 *  -t integer     : input data type (raw=0, mp4u=1)
 *  -d boolean     : save decoder output (0 False*, !=0 True)
 *  -m boolean     : save mpeg4 raw stream to single files (0 False*, !=0 True)
 *  -help          : This help message
 * (* means default)
 * 
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef WIN32
#include <sys/time.h>
#else
#include <time.h>
#endif

#include "xvid.h"

/*****************************************************************************
 *               Global vars in module and constants
 ****************************************************************************/

/* max number of frames */
#define ABS_MAXFRAMENR 9999

static int XDIM = 0;
static int YDIM = 0;
static int ARG_SAVEDECOUTPUT = 0;
static int ARG_SAVEMPEGSTREAM = 0;
static char *ARG_INPUTFILE = NULL;


static char filepath[256] = "./";
static void *dec_handle = NULL;

# define BUFFER_SIZE 10*XDIM*YDIM

/*****************************************************************************
 *               Local prototypes
 ****************************************************************************/

static double msecond();
static int write_pgm(char *filename,
		     unsigned char *image);
static int dec_init(int use_assembler);
static int dec_main(unsigned char *istream,
					unsigned char *ostream,
					int istream_size,
					int *ostream_size,
					int *isframe);
static int dec_stop();
static void usage();

/*****************************************************************************
 *        Main program
 ****************************************************************************/

int main(int argc, char *argv[])
{
	unsigned char *mp4_buffer = NULL;
	unsigned char *mp4_ptr    = NULL;
	unsigned char *out_buffer = NULL;
	int bigendian = 0;
	int still_left_in_buffer;
	int delayed_frames;
	
	double totaldectime;
  
	long totalsize;
	int status;
  
	int use_assembler = 0;
  
	char filename[256];
  
	FILE *in_file;
	int filenr;
	int i;

	printf("xvid_decraw - raw mpeg4 bitstream decoder ");
	printf("written by Christoph Lampert 2002-2003\n\n");

/*****************************************************************************
 * Command line parsing
 ****************************************************************************/

	for (i=1; i< argc; i++) {
 
		if (strcmp("-asm", argv[i]) == 0 ) {
			use_assembler = 1;
		}
		else if (strcmp("-w", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			XDIM = atoi(argv[i]);
		}
		else if (strcmp("-h", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			YDIM = atoi(argv[i]);
		}
		else if (strcmp("-d", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_SAVEDECOUTPUT = atoi(argv[i]);
		}
		else if (strcmp("-i", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_INPUTFILE = argv[i];
		}
		else if (strcmp("-m", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_SAVEMPEGSTREAM = atoi(argv[i]);
		}
		else if (strcmp("-help", argv[i])) {
			usage();
			return(0);
		}
		else {
			usage();
			exit(-1);
		}

	}
  
/*****************************************************************************
 * Values checking
 ****************************************************************************/

	if(XDIM <= 0 || XDIM > 2048 || YDIM <= 0 || YDIM > 2048) {
		usage();
		return -1;
	}

	if ( ARG_INPUTFILE == NULL || strcmp(ARG_INPUTFILE, "stdin") == 0) {
		in_file = stdin;
	}
	else {

		in_file = fopen(ARG_INPUTFILE, "rb");
		if (in_file == NULL) {
			fprintf(stderr, "Error opening input file %s\n", ARG_INPUTFILE);
			return -1;
		}
	}

/*****************************************************************************
 *        Memory allocation
 ****************************************************************************/

	/* Memory for encoded mp4 stream */
	mp4_buffer = (unsigned char *) malloc(BUFFER_SIZE);
	mp4_ptr = mp4_buffer;
	if (!mp4_buffer)
		goto free_all_memory;	
    
	/* Memory for frame output */
	out_buffer = (unsigned char *) malloc(XDIM*YDIM*4);
	if (!out_buffer)
		goto free_all_memory;	


/*****************************************************************************
 *        XviD PART  Start
 ****************************************************************************/

	status = dec_init(use_assembler);
	if (status) {
		fprintf(stderr,
			"Decore INIT problem, return value %d\n", status);
		goto release_all;
	}


/*****************************************************************************
 *	                         Main loop
 ****************************************************************************/

	/* Fill the buffer */
	still_left_in_buffer = fread(mp4_buffer, 1, BUFFER_SIZE, in_file);

	totaldectime = 0;
	totalsize = 0;
	filenr = 0;
	delayed_frames = 0;
	mp4_ptr = mp4_buffer;

	do {

		int mp4_size = (mp4_buffer + BUFFER_SIZE - mp4_ptr);
		int used_bytes = 0;
		double dectime;
		int notification;

		/*
		 * If the buffer is half empty or there are no more bytes in it
		 * then fill it.
		 */
		if (mp4_ptr > mp4_buffer + BUFFER_SIZE/2 ||
			still_left_in_buffer <= 0) {
			int rest = (mp4_buffer + BUFFER_SIZE - mp4_ptr);

			/* Move data if needed */
			if (rest)
				memcpy(mp4_buffer, mp4_ptr, rest);

			/* Update mp4_ptr */
			mp4_ptr = mp4_buffer; 

			/* read new data */
			if(feof(in_file))
				break;

			still_left_in_buffer = fread(mp4_buffer + rest,
										 1,
										 BUFFER_SIZE - rest,
										 in_file);

		}


		/* This loop flushes N_VOPS (with vop_coded bit set to 0) */
		do {

			/* Decode frame */
			dectime = msecond();
			status = dec_main(mp4_ptr, out_buffer, mp4_size, &used_bytes, &notification);
			dectime = msecond() - dectime;

			if (status) {
				break;
			}

			/* Update buffer pointers */
			mp4_ptr += used_bytes;
			still_left_in_buffer -= used_bytes;

			/* Total size */
			totalsize += used_bytes;

		}while(used_bytes <= 7 && still_left_in_buffer > 0); /* <= 7 bytes is a NVOPS */

		/* Negative buffer would mean we went too far */
		if(still_left_in_buffer < 0) break;
		
		/* Skip when decoder is buffering images or decoding VOL information */
		if(notification != XVID_DEC_VOP) {
			/* It's a delay only if it notifies NOTHING */
			if(notification == XVID_DEC_NOTHING)
				delayed_frames++;
			continue;
		}

		/* Updated data - Count only usefull decode time */
		totaldectime += dectime;

		/* Prints some decoding stats */
		printf("Frame %5d: dectime(ms) =%6.1f, length(bytes) =%7d\n", 
			   filenr, dectime, used_bytes);
			
		/* Save individual mpeg4 stream if required */
		if (ARG_SAVEMPEGSTREAM) {
			FILE *filehandle = NULL;

			sprintf(filename, "%sframe%05d.m4v", filepath, filenr);
			filehandle = fopen(filename, "wb");
			if(!filehandle) {
				fprintf(stderr,
						"Error writing single mpeg4 stream to file %s\n",
						filename);
			}
			else {
				fwrite(mp4_buffer, 1, used_bytes, filehandle);
				fclose(filehandle);
			}
		}
				
		/* Save output frame if required */
		if (ARG_SAVEDECOUTPUT) {
			sprintf(filename, "%sdec%05d.pgm", filepath, filenr);
			if(write_pgm(filename,out_buffer)) {
				fprintf(stderr,
						"Error writing decoded PGM frame %s\n",
						filename);
			}
		}

		filenr++;

	} while ( (status>=0) && (filenr<ABS_MAXFRAMENR));

/*****************************************************************************
 *     Flush decoder buffers
 ****************************************************************************/
	while(delayed_frames--) {

		/* Fake vars */
		int used_bytes, isframe;
		double dectime;

		/* Decode frame */
		dectime = msecond();
		status = dec_main(NULL, out_buffer, -1, &used_bytes, &isframe);
		dectime = msecond() - dectime;

		if (status) {
			break;
		}

		/* Updated data - Count only usefull decode time */
		totaldectime += dectime;

		/* Prints some decoding stats */
		printf("Frame %5d: dectime(ms) =%6.1f, length(bytes) =%7d\n", 
			   filenr, dectime, used_bytes);
			
		/* Save output frame if required */
		if (ARG_SAVEDECOUTPUT) {
			sprintf(filename, "%sdec%05d.pgm", filepath, filenr);
			if(write_pgm(filename,out_buffer)) {
				fprintf(stderr,
						"Error writing decoded PGM frame %s\n",
						filename);
			}
		}

		filenr++;

	}
	
/*****************************************************************************
 *     Calculate totals and averages for output, print results
 ****************************************************************************/

	totalsize    /= filenr;
	totaldectime /= filenr;
	
	printf("Avg: dectime(ms) =%7.2f, fps =%7.2f, length(bytes) =%7d\n",
		totaldectime, 1000/totaldectime, (int)totalsize);
		
/*****************************************************************************
 *      XviD PART  Stop
 ****************************************************************************/

 release_all:
  	if (dec_handle) {
	  	status = dec_stop();
		if (status)    
			fprintf(stderr, "decore RELEASE problem return value %d\n", status);
	}

 free_all_memory:
	free(out_buffer);
	free(mp4_buffer);

	return 0;
}

/*****************************************************************************
 *               Usage function
 ****************************************************************************/

static void usage()
{

	fprintf(stderr, "Usage : xvid_decraw <-w width> <-h height> [OPTIONS]\n");
	fprintf(stderr, "Options :\n");
	fprintf(stderr, " -asm           : use assembly optimizations (default=disabled)\n");
	fprintf(stderr, " -w integer     : frame width ([1.2048])\n");
	fprintf(stderr, " -h integer     : frame height ([1.2048])\n");
	fprintf(stderr, " -i string      : input filename (default=stdin)\n");
	fprintf(stderr, " -t integer     : input data type (raw=0, mp4u=1)\n");
	fprintf(stderr, " -d boolean     : save decoder output (0 False*, !=0 True)\n");
	fprintf(stderr, " -m boolean     : save mpeg4 raw stream to individual files (0 False*, !=0 True)\n");
	fprintf(stderr, " -help          : This help message\n");
	fprintf(stderr, " (* means default)\n");

}

/*****************************************************************************
 *               "helper" functions
 ****************************************************************************/

/* return the current time in milli seconds */
static double
msecond()
{	
#ifndef WIN32
	struct timeval  tv;
	gettimeofday(&tv, 0);
	return (double)tv.tv_sec*1.0e3 + (double)tv.tv_usec*1.0e-3;
#else
	clock_t clk;
	clk = clock();
	return clk * 1000 / CLOCKS_PER_SEC;
#endif
}

/*****************************************************************************
 *              output functions
 ****************************************************************************/

static int
write_pgm(char *filename,
	  unsigned char *image)
{
	int loop;

	unsigned char *y = image;
	unsigned char *u = image + XDIM*YDIM;
	unsigned char *v = image + XDIM*YDIM + XDIM/2*YDIM/2;

	FILE *filehandle;
	filehandle=fopen(filename,"w+b");
	if (filehandle) {

		/* Write header */
		fprintf(filehandle,"P5\n\n%d %d 255\n", XDIM,YDIM*3/2);

		/* Write Y data */
		fwrite(y, 1, XDIM*YDIM, filehandle);

		for(loop=0; loop<YDIM/2; loop++)
		{
			/* Write U scanline */
			fwrite(u, 1, XDIM/2, filehandle);

			/* Write V scanline */
			fwrite(v, 1, XDIM/2, filehandle);

			/* Update pointers */
			u += XDIM/2;
			v += XDIM/2;

		}

		/* Close file */
		fclose(filehandle);

		return 0;
	}
	else
		return 1;
}

/*****************************************************************************
 * Routines for decoding: init decoder, use, and stop decoder
 ****************************************************************************/

/* init decoder before first run */
static int
dec_init(int use_assembler)
{
        int xerr;

        XVID_INIT_PARAM xinit;
        XVID_DEC_PARAM xparam;

		if(use_assembler)
#ifdef ARCH_IS_IA64
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

/* decode one frame  */
static int
dec_main(unsigned char *istream,
		 unsigned char *ostream,
		 int istream_size,
		 int *ostream_size,
		 int *notification)
{

        int xerr;
        XVID_DEC_FRAME xframe;
		XVID_DEC_STATS xstats;

        xframe.general    = 0;
		xframe.bitstream  = istream;
        xframe.length     = istream_size;
		xframe.image      = ostream;
        xframe.stride     = XDIM;
		xframe.colorspace = XVID_CSP_YV12;             

		xerr = xvid_decore(dec_handle, XVID_DEC_DECODE, &xframe, &xstats);

		*ostream_size = xframe.length;

		*notification = xstats.notify;

        return xerr;
}

/* close decoder to release resources */
static int
dec_stop()
{
        int xerr;

        xerr = xvid_decore(dec_handle, XVID_DEC_DESTROY, NULL, NULL);

        return xerr;
}
