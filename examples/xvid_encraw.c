/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Console based test application  -
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
 * $Id: xvid_encraw.c,v 1.12 2003-02-22 21:36:27 chl Exp $
 *
 ****************************************************************************/

/*****************************************************************************
 *  Application notes :
 *		                    
 *  A sequence of YUV pics in PGM file format is encoded and decoded
 *  The speed is measured and PSNR of decoded picture is calculated. 
 *		                   
 *  The program is plain C and needs no libraries except for libxvidcore, 
 *  and maths-lib.
 *	
 ************************************************************************/

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
 *                            Quality presets
 ****************************************************************************/

static int const motion_presets[7] = {
	0,                                                        /* Q 0 */
	PMV_EARLYSTOP16,                                          /* Q 1 */
	PMV_EARLYSTOP16,                                          /* Q 2 */
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,                    /* Q 3 */
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,                    /* Q 4 */
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EARLYSTOP8 |  /* Q 5 */
	PMV_HALFPELREFINE8,
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EXTSEARCH16 | /* Q 6 */
	PMV_USESQUARES16 | PMV_EARLYSTOP8 | PMV_HALFPELREFINE8
};

static int const general_presets[7] = {
	XVID_H263QUANT,	                              /* Q 0 */
	XVID_MPEGQUANT,                               /* Q 1 */
	XVID_H263QUANT,                               /* Q 2 */
	XVID_H263QUANT | XVID_HALFPEL,                /* Q 3 */
	XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V, /* Q 4 */
	XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V, /* Q 5 */
	XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V  /* Q 6 */
};
		

/*****************************************************************************
 *                     Command line global variables
 ****************************************************************************/

/* Maximum number of frames to encode */
#define ABS_MAXFRAMENR 9999

/* HINTMODEs */
#define HINT_MODE_NONE 0
#define HINT_MODE_GET  1
#define HINT_MODE_SET  2
#define HINT_FILE "hints.mv"

static int   ARG_BITRATE = 900;
static int   ARG_QUANTI = 0;
static int   ARG_QUALITY = 6;
static int   ARG_MINQUANT = 1;
static int   ARG_MAXQUANT = 31;
static float ARG_FRAMERATE = 25.00f;
static int   ARG_MAXFRAMENR = ABS_MAXFRAMENR;
static char *ARG_INPUTFILE = NULL;
static int   ARG_INPUTTYPE = 0;
static int   ARG_SAVEMPEGSTREAM = 0;
static char *ARG_OUTPUTFILE = NULL;
static int   ARG_HINTMODE = HINT_MODE_NONE;
static int   XDIM = 0;
static int   YDIM = 0;
static int   ARG_BQRATIO = 120;
static int   ARG_BQOFFSET = 0;
static int   ARG_MAXBFRAMES = 0;
#define IMAGE_SIZE(x,y) ((x)*(y)*3/2)

#define MAX(A,B) ( ((A)>(B)) ? (A) : (B) )
#define SMALL_EPS 1e-10

#define SWAP(a) ( (((a)&0x000000ff)<<24) | (((a)&0x0000ff00)<<8) | \
                  (((a)&0x00ff0000)>>8)  | (((a)&0xff000000)>>24) )

/****************************************************************************
 *                     Nasty global vars ;-)
 ***************************************************************************/

static int i,filenr = 0;

/* the path where to save output */
static char filepath[256] = "./";

/* Internal structures (handles) for encoding and decoding */
static void *enc_handle = NULL;

/*****************************************************************************
 *               Local prototypes
 ****************************************************************************/

/* Prints program usage message */
static void usage();

/* Statistical functions */
static double msecond();

/* PGM related functions */
static int read_pgmheader(FILE* handle);
static int read_pgmdata(FILE* handle, unsigned char *image);
static int read_yuvdata(FILE* handle, unsigned char *image);

/* Encoder related functions */
static int enc_init(int use_assembler);
static int enc_stop();
static int enc_main(unsigned char* image, unsigned char* bitstream,
					unsigned char *hints_buffer,
					long *streamlength, long* frametype, long *hints_size);

/*****************************************************************************
 *               Main function
 ****************************************************************************/

int main(int argc, char *argv[])
{

	unsigned char *mp4_buffer = NULL;
	unsigned char *in_buffer = NULL;
	unsigned char *out_buffer = NULL;
	unsigned char *hints_buffer = NULL;

	double enctime;
	double totalenctime=0.;
  
	long totalsize;
	long hints_size;
	int status;
	long frame_type;
	long bigendian;
  
	long m4v_size;
	int use_assembler=0;
  
	char filename[256];
  
	FILE *in_file = stdin;
	FILE *out_file = NULL;
	FILE *hints_file = NULL;

	printf("xvid_encraw - raw mpeg4 bitstream encoder ");
	printf("written by Christoph Lampert 2002-2003\n\n");

/*****************************************************************************
 *                            Command line parsing
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
		else if (strcmp("-b", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_BITRATE = atoi(argv[i]);
		}
		else if (strcmp("-bn", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_MAXBFRAMES = atoi(argv[i]);
		}
		else if (strcmp("-bqr", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_BQRATIO = atoi(argv[i]);
		}
		else if (strcmp("-bqo", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_BQOFFSET = atoi(argv[i]);
		}
		else if (strcmp("-q", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_QUALITY = atoi(argv[i]);
		}
		else if (strcmp("-f", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_FRAMERATE = (float)atof(argv[i]);
		}
		else if (strcmp("-i", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_INPUTFILE = argv[i];
		}
		else if (strcmp("-t", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_INPUTTYPE = atoi(argv[i]);
		}
		else if(strcmp("-n", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_MAXFRAMENR  = atoi(argv[i]);
		}
		else if (strcmp("-quant", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_QUANTI = atoi(argv[i]);
		}
		else if (strcmp("-m", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_SAVEMPEGSTREAM = atoi(argv[i]);
		}
		else if (strcmp("-mv", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_HINTMODE = atoi(argv[i]);
		}
		else if (strcmp("-o", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_OUTPUTFILE = argv[i];
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
 *                            Arguments checking
 ****************************************************************************/

	if (XDIM <= 0 || XDIM >= 2048 || YDIM <=0 || YDIM >= 2048 ) {
		fprintf(stderr,	"Trying to retreive width and height from PGM header\n");
		ARG_INPUTTYPE = 1; /* pgm */
	}
  
	if ( ARG_QUALITY < 0 || ARG_QUALITY > 6) {
		fprintf(stderr,"Wrong Quality\n");
		return -1;
	}

	if ( ARG_BITRATE <= 0 && ARG_QUANTI == 0) {
		fprintf(stderr,"Wrong Bitrate\n");
		return -1;
	}

	if ( ARG_FRAMERATE <= 0) {
		fprintf(stderr,"Wrong Framerate %s \n",argv[5]);
		return -1;
	}

	if ( ARG_MAXFRAMENR <= 0) {
		fprintf(stderr,"Wrong number of frames\n");
		return -1;
	}

	if ( ARG_HINTMODE != HINT_MODE_NONE &&
		 ARG_HINTMODE != HINT_MODE_GET &&
		 ARG_HINTMODE != HINT_MODE_SET)
		ARG_HINTMODE = HINT_MODE_NONE;

	if( ARG_HINTMODE != HINT_MODE_NONE) {
		char *rights = "rb";

		/*
		 * If we are getting hints from core, we will have to write them to
		 * hint file
		 */
		if(ARG_HINTMODE == HINT_MODE_GET)
			rights = "w+b";

		/* Open the hint file */
		hints_file = fopen(HINT_FILE, rights);
		if(hints_file == NULL) {
			fprintf(stderr, "Error opening input file %s\n", HINT_FILE);
			return -1;
		}

		/* Allocate hint memory space, we will be using rawhints */
		/* NB : Hope 1Mb is enough */
		if((hints_buffer = malloc(1024*1024)) == NULL) {
			fprintf(stderr, "Memory allocation error\n");
			return -1;
		}

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

	if (ARG_INPUTTYPE) {
		if (read_pgmheader(in_file)) {
			fprintf(stderr, "Wrong input format, I want YUV encapsulated in PGM\n");
			return -1;
		}
	}

	/* now we know the sizes, so allocate memory */
	in_buffer = (unsigned char *) malloc(IMAGE_SIZE(XDIM,YDIM));
	if (!in_buffer)
		goto free_all_memory;

	/* this should really be enough memory ! */
	mp4_buffer = (unsigned char *) malloc(IMAGE_SIZE(XDIM,YDIM)*2);
	if (!mp4_buffer)
		goto free_all_memory;	

/*****************************************************************************
 *                            XviD PART  Start
 ****************************************************************************/


	status = enc_init(use_assembler);
	if (status)    
	{ 
		fprintf(stderr, "Encore INIT problem, return value %d\n", status);
		goto release_all;
	}

/*****************************************************************************
 *                            Main loop
 ****************************************************************************/

	if (ARG_SAVEMPEGSTREAM && ARG_OUTPUTFILE) {

		if((out_file = fopen(ARG_OUTPUTFILE, "w+b")) == NULL) {
			fprintf(stderr, "Error opening output file %s\n", ARG_OUTPUTFILE);
			goto release_all;
		}

	}
	else {
		out_file = NULL;
	}

/*****************************************************************************
 *                       Encoding loop
 ****************************************************************************/

	totalsize = 0;

	do {

		if (ARG_INPUTTYPE)
			status = read_pgmdata(in_file, in_buffer);	/* read PGM data (YUV-format) */
		else
			status = read_yuvdata(in_file, in_buffer);	/* read raw data (YUV-format) */
	      
		if (status)
		{
			/* Couldn't read image, most likely end-of-file */
			continue;
		}

/*****************************************************************************
 *                       Read hints from file
 ****************************************************************************/

		if(ARG_HINTMODE == HINT_MODE_SET) {
			fread(&hints_size, 1, sizeof(long), hints_file);
			hints_size = (!bigendian)?SWAP(hints_size):hints_size;
			fread(hints_buffer, 1, hints_size, hints_file);
		}

/*****************************************************************************
 *                       Encode and decode this frame
 ****************************************************************************/

		enctime = msecond();
		status = enc_main(in_buffer, mp4_buffer, hints_buffer,
						  &m4v_size, &frame_type, &hints_size);
		enctime = msecond() - enctime;

		/* if it's a not coded VOP (aka NVOP) then we write nothing */
		if(frame_type == 5) goto next_frame;

		{
			char *type[] = {"P", "I", "B", "S", "Packed", "N", "Unknown"};

			if(frame_type<0 || frame_type>5) frame_type = 6;			

			printf("Frame %5d: type = %s, enctime(ms) =%6.1f, length(bytes) =%7d\n",
				   (int)filenr, type[frame_type], (float)enctime, (int)m4v_size);

		}

		/* Update encoding time stats */
		totalenctime += enctime;
		totalsize += m4v_size;

/*****************************************************************************
 *                       Save hints to file
 ****************************************************************************/

		if(ARG_HINTMODE == HINT_MODE_GET) {
			hints_size = (!bigendian)?SWAP(hints_size):hints_size;
			fwrite(&hints_size, 1, sizeof(long), hints_file);
			hints_size = (!bigendian)?SWAP(hints_size):hints_size;
			fwrite(hints_buffer, 1, hints_size, hints_file);
		}

/*****************************************************************************
 *                       Save stream to file
 ****************************************************************************/

		if (ARG_SAVEMPEGSTREAM)
		{
			/* Save single files */
			if (out_file == NULL) {
				sprintf(filename, "%sframe%05d.m4v", filepath, filenr);
				out_file = fopen(filename, "wb");
				fwrite(mp4_buffer, m4v_size, 1, out_file);
				fclose(out_file);
				out_file = NULL;
			}
			else {

				/* Write mp4 data */
				fwrite(mp4_buffer, 1, m4v_size, out_file);

			}
		}

	next_frame:
		/* Read the header if it's pgm stream */ 
		if (ARG_INPUTTYPE)
			status = read_pgmheader(in_file);

		if(frame_type != 5) filenr++;

	} while ( (!status) && (filenr<ARG_MAXFRAMENR) );

	
      
/*****************************************************************************
 *         Calculate totals and averages for output, print results
 ****************************************************************************/

	totalsize    /= filenr;
	totalenctime /= filenr;

	printf("Avg: enctime(ms) =%7.2f, fps =%7.2f, length(bytes) = %7d\n",
		   totalenctime, 1000/totalenctime, (int)totalsize);

/*****************************************************************************
 *                            XviD PART  Stop
 ****************************************************************************/

 release_all:

	if (enc_handle)
	{	
		status = enc_stop();
		if (status)    
			fprintf(stderr, "Encore RELEASE problem return value %d\n", status);
	}

	if(in_file)
		fclose(in_file);
	if(out_file)
		fclose(out_file);
	if(hints_file)
		fclose(hints_file);

 free_all_memory:
	free(out_buffer);
	free(mp4_buffer);
	free(in_buffer);
	if(hints_buffer) free(hints_buffer);

	return 0;

}


/*****************************************************************************
 *                        "statistical" functions
 *
 *  these are not needed for encoding or decoding, but for measuring
 *  time and quality, there in nothing specific to XviD in these
 *
 *****************************************************************************/

/* Return time elapsed time in miliseconds since the program started */
static double msecond()
{	
#ifndef WIN32
	struct timeval  tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec*1.0e3 + tv.tv_usec * 1.0e-3;
#else
	clock_t clk;
	clk = clock();
	return clk * 1000 / CLOCKS_PER_SEC;
#endif
}

/*****************************************************************************
 *                             Usage message
 *****************************************************************************/

static void usage()
{

	fprintf(stderr, "Usage : xvid_stat [OPTIONS]\n");
	fprintf(stderr, "Options :\n");
	fprintf(stderr, " -w integer     : frame width ([1.2048])\n");
	fprintf(stderr, " -h integer     : frame height ([1.2048])\n");
	fprintf(stderr, " -b integer     : target bitrate (>0 | default=900kbit)\n");
	fprintf(stderr, " -b integer     : target bitrate (>0 | default=900kbit)\n");
	fprintf(stderr, " -bn integer    : max bframes (default=0)\n");
	fprintf(stderr, " -bqr integer   : bframe quantizer ratio (default=150)\n");
	fprintf(stderr, " -bqo integer   : bframe quantizer offset (default=100)\n");
	fprintf(stderr, " -f float       : target framerate (>0)\n");
	fprintf(stderr, " -i string      : input filename (default=stdin)\n");
	fprintf(stderr, " -t integer     : input data type (yuv=0, pgm=1)\n");
	fprintf(stderr, " -n integer     : number of frames to encode\n");
	fprintf(stderr, " -q integer     : quality ([0..5])\n");
	fprintf(stderr, " -d boolean     : save decoder output (0 False*, !=0 True)\n");
	fprintf(stderr, " -m boolean     : save mpeg4 raw stream (0 False*, !=0 True)\n");
	fprintf(stderr, " -o string      : output container filename (only usefull when -m 1 is used) :\n");
	fprintf(stderr, "                  When this option is not used : one file per encoded frame\n");
	fprintf(stderr, "                  When this option is used : save to 'string' (default=stream.m4v)\n");
	fprintf(stderr, " -mt integer    : output type (m4v=0, mp4u=1)\n");
	fprintf(stderr, " -mv integer    : Use motion vector hints (no hints=0, get hints=1, set hints=2)\n");
	fprintf(stderr, " -help          : prints this help message\n");
	fprintf(stderr, " -quant integer : fixed quantizer (disables -b setting)\n");
	fprintf(stderr, " (* means default)\n");

}

/*****************************************************************************
 *                       Input and output functions
 *
 *      the are small and simple routines to read and write PGM and YUV
 *      image. It's just for convenience, again nothing specific to XviD
 *
 *****************************************************************************/

static int read_pgmheader(FILE* handle)
{	
	int bytes,xsize,ysize,depth;
	char dummy[2];
	
	bytes = fread(dummy,1,2,handle);

	if ( (bytes < 2) || (dummy[0] != 'P') || (dummy[1] != '5' ))
   		return 1;

	fscanf(handle,"%d %d %d",&xsize,&ysize,&depth); 
	if ( (xsize > 1440) || (ysize > 2880 ) || (depth != 255) )
	{
		fprintf(stderr,"%d %d %d\n",xsize,ysize,depth);
	   	return 2;
	}
	if ( (XDIM==0) || (YDIM==0) )
	{
		XDIM=xsize;
		YDIM=ysize*2/3;
	}

	return 0;
}

static int read_pgmdata(FILE* handle, unsigned char *image)
{	
	int i;
	char dummy;
	
	unsigned char *y = image;
	unsigned char *u = image + XDIM*YDIM;
	unsigned char *v = image + XDIM*YDIM + XDIM/2*YDIM/2; 

	/* read Y component of picture */
	fread(y, 1, XDIM*YDIM, handle);
 
	for (i=0;i<YDIM/2;i++)
	{
		/* read U */
		fread(u, 1, XDIM/2, handle);

		/* read V */
		fread(v, 1, XDIM/2, handle);

		/* Update pointers */
		u += XDIM/2;
		v += XDIM/2;
	}

    /*  I don't know why, but this seems needed */
	fread(&dummy, 1, 1, handle);

	return 0;
}

static int read_yuvdata(FILE* handle, unsigned char *image)
{
   
	if (fread(image, 1, IMAGE_SIZE(XDIM, YDIM), handle) != (unsigned int)IMAGE_SIZE(XDIM, YDIM)) 
		return 1;
	else	
		return 0;
}

/*****************************************************************************
 *     Routines for encoding: init encoder, frame step, release encoder
 ****************************************************************************/

#define FRAMERATE_INCR 1001

/* Initialize encoder for first use, pass all needed parameters to the codec */
static int enc_init(int use_assembler)
{
	int xerr;
	
	XVID_INIT_PARAM xinit;
	XVID_ENC_PARAM xparam;

	if(use_assembler) {

#ifdef ARCH_IS_IA64
		xinit.cpu_flags = XVID_CPU_FORCE | XVID_CPU_IA64;
#else
		xinit.cpu_flags = 0;
#endif
	}
	else {
		xinit.cpu_flags = XVID_CPU_FORCE;
	}

	xvid_init(NULL, 0, &xinit, NULL);

	xparam.width = XDIM;
	xparam.height = YDIM;
	if ((ARG_FRAMERATE - (int)ARG_FRAMERATE) < SMALL_EPS)
	{
		xparam.fincr = 1;
		xparam.fbase = (int)ARG_FRAMERATE;
	}
	else
	{
		xparam.fincr = FRAMERATE_INCR;
		xparam.fbase = (int)(FRAMERATE_INCR * ARG_FRAMERATE);
	}
	xparam.rc_reaction_delay_factor = 16;
    xparam.rc_averaging_period = 100;
    xparam.rc_buffer = 10;
	xparam.rc_bitrate = ARG_BITRATE*1000; 
	xparam.min_quantizer = ARG_MINQUANT;
	xparam.max_quantizer = ARG_MAXQUANT;
	xparam.max_key_interval = (int)ARG_FRAMERATE*10;
	xparam.bquant_ratio = ARG_BQRATIO;
	xparam.bquant_offset = ARG_BQOFFSET;	
	xparam.max_bframes = ARG_MAXBFRAMES;
	xparam.frame_drop_ratio = 0;
	xparam.global = 0;

	/* I use a small value here, since will not encode whole movies, but short clips */

	xerr = xvid_encore(NULL, XVID_ENC_CREATE, &xparam, NULL);
	enc_handle=xparam.handle;

	return xerr;
}

static int enc_stop()
{
	int xerr;

	xerr = xvid_encore(enc_handle, XVID_ENC_DESTROY, NULL, NULL);
	return xerr;

}

static int enc_main(unsigned char* image, unsigned char* bitstream,
					unsigned char* hints_buffer,
					long *streamlength, long *frametype, long *hints_size)
{
	int xerr;

	XVID_ENC_FRAME xframe;
	XVID_ENC_STATS xstats;

	xframe.bitstream = bitstream;
	xframe.length = -1; 	/* this is written by the routine */

	xframe.image = image;
	xframe.colorspace = XVID_CSP_I420;	/* defined in <xvid.h> */

	xframe.intra = -1; /* let the codec decide between I-frame (1) and P-frame (0) */

	xframe.quant = ARG_QUANTI;	/* is quant != 0, use a fixed quant (and ignore bitrate) */
	xframe.bquant = 0;
	
	xframe.motion = motion_presets[ARG_QUALITY];
	xframe.general = general_presets[ARG_QUALITY];
	xframe.quant_intra_matrix = xframe.quant_inter_matrix = NULL;
	xframe.stride = XDIM;

	xframe.hint.hintstream = hints_buffer;

	if(ARG_HINTMODE == HINT_MODE_SET) {
		xframe.hint.hintlength = *hints_size;
		xframe.hint.rawhints = 0;
		xframe.general |= XVID_HINTEDME_SET;
	}

	if(ARG_HINTMODE == HINT_MODE_GET) {
		xframe.hint.rawhints = 0;
		xframe.general |= XVID_HINTEDME_GET;
	}

	xerr = xvid_encore(enc_handle, XVID_ENC_ENCODE, &xframe, &xstats);

	if(ARG_HINTMODE == HINT_MODE_GET)
		*hints_size = xframe.hint.hintlength;

	/*
	 * This is statictical data, e.g. for 2-pass. If you are not
	 * interested in any of this, you can use NULL instead of &xstats
	 */
	*frametype = xframe.intra;
	*streamlength = xframe.length;

	return xerr;
}
