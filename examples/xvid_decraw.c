/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Console based decoding test application  -
 *
 *  Copyright(C) 2002-2003 Christoph Lampert
 *               2002-2003 Edouard Gomez <ed.gomez@free.fr>
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
 * $Id: xvid_decraw.c,v 1.10 2004-03-22 22:36:23 edgomez Exp $
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
 *  and maths-lib.
 *		                   
 *  Use ./xvid_decraw -help for a list of options
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

#define BUFFER_SIZE (2*1024*1024)

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
					xvid_dec_stats_t *xvid_dec_stats);
static int dec_stop();
static void usage();


const char * type2str(int type)
{
    if (type==XVID_TYPE_IVOP)
        return "I";
    if (type==XVID_TYPE_PVOP)
        return "P";
    if (type==XVID_TYPE_BVOP)
        return "B";
    return "S";
}

/*****************************************************************************
 *        Main program
 ****************************************************************************/

int main(int argc, char *argv[])
{
	unsigned char *mp4_buffer = NULL;
	unsigned char *mp4_ptr    = NULL;
	unsigned char *out_buffer = NULL;
	int useful_bytes;
	xvid_dec_stats_t xvid_dec_stats;
	
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
		} else if (strcmp("-d", argv[i]) == 0) {
			ARG_SAVEDECOUTPUT = 1;
		} else if (strcmp("-i", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_INPUTFILE = argv[i];
		} else if (strcmp("-m", argv[i]) == 0) {
			ARG_SAVEMPEGSTREAM = 1;
		} else if (strcmp("-help", argv[i]) == 0) {
			usage();
			return(0);
		} else {
			usage();
			exit(-1);
		}
	}
  
/*****************************************************************************
 * Values checking
 ****************************************************************************/

	if ( ARG_INPUTFILE == NULL || strcmp(ARG_INPUTFILE, "stdin") == 0) {
		in_file = stdin;
	}
	else {

		in_file = fopen(ARG_INPUTFILE, "rb");
		if (in_file == NULL) {
			fprintf(stderr, "Error opening input file %s\n", ARG_INPUTFILE);
			return(-1);
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
	useful_bytes = fread(mp4_buffer, 1, BUFFER_SIZE, in_file);

	totaldectime = 0;
	totalsize = 0;
	filenr = 0;
	mp4_ptr = mp4_buffer;

	do {
		int used_bytes = 0;
		double dectime;

		/*
		 * If the buffer is half empty or there are no more bytes in it
		 * then fill it.
		 */
		if (mp4_ptr > mp4_buffer + BUFFER_SIZE/2) {
			int already_in_buffer = (mp4_buffer + BUFFER_SIZE - mp4_ptr);

			/* Move data if needed */
			if (already_in_buffer > 0)
				memcpy(mp4_buffer, mp4_ptr, already_in_buffer);

			/* Update mp4_ptr */
			mp4_ptr = mp4_buffer; 

			/* read new data */
            if(feof(in_file))
				break;

			useful_bytes += fread(mp4_buffer + already_in_buffer,
								  1, BUFFER_SIZE - already_in_buffer,
								  in_file);

		}


		/* This loop is needed to handle VOL/NVOP reading */
		do {

			/* Decode frame */
			dectime = msecond();
			used_bytes = dec_main(mp4_ptr, out_buffer, useful_bytes, &xvid_dec_stats);
			dectime = msecond() - dectime;

			/* Resize image buffer if needed */
			if(xvid_dec_stats.type == XVID_TYPE_VOL) {

				/* Check if old buffer is smaller */
				if(XDIM*YDIM < xvid_dec_stats.data.vol.width*xvid_dec_stats.data.vol.height) {

					/* Copy new witdh and new height from the vol structure */
					XDIM = xvid_dec_stats.data.vol.width;
					YDIM = xvid_dec_stats.data.vol.height;

					/* Free old output buffer*/
					if(out_buffer) free(out_buffer);

					/* Allocate the new buffer */
					out_buffer = (unsigned char*)malloc(XDIM*YDIM*4);
					if(out_buffer == NULL)
						goto free_all_memory;

					fprintf(stderr, "Resized frame buffer to %dx%d\n", XDIM, YDIM);
				}
			}

			/* Update buffer pointers */
			if(used_bytes > 0) {
				mp4_ptr += used_bytes;
				useful_bytes -= used_bytes;

				/* Total size */
				totalsize += used_bytes;
			}

		}while(xvid_dec_stats.type <= 0 && useful_bytes > 0);

		/* Check if there is a negative number of useful bytes left in buffer
		 * This means we went too far */
        if(useful_bytes < 0)
            break;
		
    	/* Updated data - Count only usefull decode time */
		totaldectime += dectime;

			
        printf("Frame %5d: type = %s, dectime(ms) =%6.1f, length(bytes) =%7d\n",
			   filenr, type2str(xvid_dec_stats.type), dectime, used_bytes);
			
		/* Save individual mpeg4 stream if required */
		if(ARG_SAVEMPEGSTREAM) {
			FILE *filehandle = NULL;

			sprintf(filename, "%sframe%05d.m4v", filepath, filenr);
			filehandle = fopen(filename, "wb");
			if(!filehandle) {
				fprintf(stderr,
						"Error writing single mpeg4 stream to file %s\n",
						filename);
			}
			else {
				fwrite(mp4_ptr-used_bytes, 1, used_bytes, filehandle);
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

	do {

		/* Fake vars */
		int used_bytes;
		double dectime;

        do {
		    dectime = msecond();
		    used_bytes = dec_main(NULL, out_buffer, -1, &xvid_dec_stats);
		    dectime = msecond() - dectime;
        }while(used_bytes>=0 && xvid_dec_stats.type <= 0);

        if (used_bytes < 0) {   /* XVID_ERR_END */
            break;
        }

		/* Updated data - Count only usefull decode time */
		totaldectime += dectime;

		/* Prints some decoding stats */
        printf("Frame %5d: type = %s, dectime(ms) =%6.1f, length(bytes) =%7d\n",
			   filenr, type2str(xvid_dec_stats.type), dectime, used_bytes);
			
		/* Save output frame if required */
		if (ARG_SAVEDECOUTPUT) {
			sprintf(filename, "%sdec%05d.pgm", filepath, filenr);
			if(write_pgm(filename, out_buffer)) {
				fprintf(stderr,
						"Error writing decoded PGM frame %s\n",
						filename);
			}
		}

		filenr++;

	}while(1);
	
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

	return(0);
}

/*****************************************************************************
 *               Usage function
 ****************************************************************************/

static void usage()
{

	fprintf(stderr, "Usage : xvid_decraw [OPTIONS]\n");
	fprintf(stderr, "Options :\n");
	fprintf(stderr, " -asm           : use assembly optimizations (default=disabled)\n");
	fprintf(stderr, " -i string      : input filename (default=stdin)\n");
	fprintf(stderr, " -d             : save decoder output\n");
	fprintf(stderr, " -m             : save mpeg4 raw stream to individual files\n");
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
	return((double)tv.tv_sec*1.0e3 + (double)tv.tv_usec*1.0e-3);
#else
	clock_t clk;
	clk = clock();
	return(clk * 1000 / CLOCKS_PER_SEC);
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

		return(0);
	}
	else
		return(1);
}

/*****************************************************************************
 * Routines for decoding: init decoder, use, and stop decoder
 ****************************************************************************/

/* init decoder before first run */
static int
dec_init(int use_assembler)
{
	int ret;

	xvid_gbl_init_t   xvid_gbl_init;
	xvid_dec_create_t xvid_dec_create;

	/*------------------------------------------------------------------------
	 * XviD core initialization
	 *----------------------------------------------------------------------*/

	/* Version */
	xvid_gbl_init.version = XVID_VERSION;

	/* Assembly setting */
	if(use_assembler)
#ifdef ARCH_IS_IA64
		xvid_gbl_init.cpu_flags = XVID_CPU_FORCE | XVID_CPU_IA64;
#else
	xvid_gbl_init.cpu_flags = 0;
#endif
	else
		xvid_gbl_init.cpu_flags = XVID_CPU_FORCE;

	xvid_global(NULL, 0, &xvid_gbl_init, NULL);

	/*------------------------------------------------------------------------
	 * XviD encoder initialization
	 *----------------------------------------------------------------------*/

	/* Version */
	xvid_dec_create.version = XVID_VERSION;

	/*
	 * Image dimensions -- set to 0, xvidcore will resize when ever it is
	 * needed
	 */
	xvid_dec_create.width = 0;
	xvid_dec_create.height = 0;

	ret = xvid_decore(NULL, XVID_DEC_CREATE, &xvid_dec_create, NULL);

	dec_handle = xvid_dec_create.handle;

	return(ret);
}

/* decode one frame  */
static int
dec_main(unsigned char *istream,
		 unsigned char *ostream,
		 int istream_size,
		 xvid_dec_stats_t *xvid_dec_stats)
{

	int ret;

	xvid_dec_frame_t xvid_dec_frame;

	/* Set version */
	xvid_dec_frame.version = XVID_VERSION;
	xvid_dec_stats->version = XVID_VERSION;

	/* No general flags to set */
	xvid_dec_frame.general          = 0;

	/* Input stream */
	xvid_dec_frame.bitstream        = istream;
	xvid_dec_frame.length           = istream_size;

	/* Output frame structure */
	xvid_dec_frame.output.plane[0]  = ostream;
	xvid_dec_frame.output.stride[0] = XDIM;
	xvid_dec_frame.output.csp       = XVID_CSP_I420;

	ret = xvid_decore(dec_handle, XVID_DEC_DECODE, &xvid_dec_frame, xvid_dec_stats);

	return(ret);
}

/* close decoder to release resources */
static int
dec_stop()
{
	int ret;

	ret = xvid_decore(dec_handle, XVID_DEC_DESTROY, NULL, NULL);

	return(ret);
}
