/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Console based test application  -
 *
 *  Copyright(C) 2002 Christoph Lampert
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
 * $Id: xvid_stat.c,v 1.8 2002-09-15 20:43:52 edgomez Exp $
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
 * Usage : xvid_stat [OPTIONS]
 * Options :
 *  -w integer     : frame width ([1.2048])
 *  -h integer     : frame height ([1.2048])
 *  -b integer     : target bitrate (>0 | default=900kbit)
 *  -f float       : target framerate (>0)
 *  -i string      : input filename (default=stdin)
 *  -t integer     : input data type (yuv=0, pgm=1)
 *  -n integer     : number of frames to encode
 *  -q integer     : quality ([0..5])
 *  -d boolean     : save decoder output (0 False*, !=0 True)
 *  -m boolean     : save mpeg4 raw stream (0 False*, !=0 True)
 *  -h, -help      : prints this help message
 *  -quant integer : fixed quantizer (disables -b setting)
 *  (* means default)
 *
 *  An input file named "stdin" is treated as standard input stream.
 *
 *
 *  PGM input must be in a very specific format, basically it pgm file must
 *  contain Y plane first just like usual P5 pgm files, and then U and V
 *  planes are stored just after Y plane so y dimension is y*3/2 in reality
 *
 *  See read_pgmheader for more details.
 *
 *  Such a PGM file can be generated from MPEG2 by # mpeg2dec -o pgmpipe
 *	
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "../src/xvid.h"

/****************************************************************************
 *                               Prototypes
 ***************************************************************************/

/* Prints program usage message */
static void usage();

/* Statistical functions */
static double msecond();
static double absdistq(int x, int y,
					   unsigned char* buf1, int stride1,
					   unsigned char* buf2, int stride2);
static double PSNR(int x, int y,
				   unsigned char* buf1, int stride1,
				   unsigned char* buf2, int stride2);

/* PGM related functions */
static int read_pgmheader(FILE* handle);
static int read_pgmdata(FILE* handle, unsigned char *image);
static int read_yuvdata(FILE* handle, unsigned char *image);
static int write_pgm(char *filename, unsigned char *image);

/* Encoder related functions */
static int enc_init(int use_assembler);
static int enc_stop();
static int enc_main(unsigned char* image, unsigned char* bitstream,
					int *streamlength, int* frametype);

/* Decoder related functions */
static int dec_stop();
static int dec_main(unsigned char *m4v_buffer, unsigned char *out_buffer,
					int m4v_size);
static int dec_init(int use_assembler);

/*****************************************************************************
 *                            Quality presets
 ****************************************************************************/

static int const motion_presets[7] = {
	0,                                                        // Q 0
	PMV_EARLYSTOP16,                                          // Q 1
	PMV_EARLYSTOP16,                                          // Q 2
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,                    // Q 3
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,                    // Q 4
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EARLYSTOP8 |  // Q 5
	PMV_HALFPELREFINE8,
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EXTSEARCH16 | // Q 6
	PMV_USESQUARES16 | PMV_EARLYSTOP8 | PMV_HALFPELREFINE8
};

static int const general_presets[7] = {
	XVID_H263QUANT,	                              // Q 0
	XVID_MPEGQUANT,                               // Q 1
	XVID_H263QUANT,                               // Q 2
	XVID_H263QUANT | XVID_HALFPEL,                // Q 3
	XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V, // Q 4
	XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V, // Q 5
	XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V  // Q 6
};
		

/*****************************************************************************
 *                     Command line global variables
 ****************************************************************************/

/* Maximum number of frames to encode */
#define ABS_MAXFRAMENR 9999

static int   ARG_BITRATE = 900;
static int   ARG_QUANTI = 0;
static int   ARG_QUALITY = 6;
static int   ARG_MINQUANT = 1;
static int   ARG_MAXQUANT = 31;
static float ARG_FRAMERATE = 25.00f;
static int   ARG_MAXFRAMENR = ABS_MAXFRAMENR;
static char *ARG_INPUTFILE = NULL;
static int   ARG_INPUTTYPE = 0;
static int   ARG_SAVEDECOUTPUT = 0;
static int   ARG_SAVEMPEGSTREAM = 0;
static int   XDIM = 0;
static int   YDIM = 0;
#define IMAGE_SIZE(x,y) ((x)*(y)*3/2)

#define MAX(A,B) ( ((A)>(B)) ? (A) : (B) )
#define SMALL_EPS 1e-10

/****************************************************************************
 *                     Nasty global vars ;-)
 ***************************************************************************/

static int i,filenr = 0;
static int save_ref_flag = 0;

/* the path where to save output */
static char filepath[256] = "./";

/* Internal structures (handles) for encoding and decoding */
static void *enc_handle = NULL;
static void *dec_handle = NULL;

/*****************************************************************************
 *                               Main program
 ****************************************************************************/

int main(int argc, char *argv[])
{

	unsigned char *divx_buffer = NULL;
	unsigned char *in_buffer = NULL;
	unsigned char *out_buffer = NULL;

	double enctime,dectime;
	double totalenctime=0.;
	double totaldectime=0.;
  
	long totalsize=0;
	int status;
  
	int m4v_size;
	int frame_type[ABS_MAXFRAMENR];
	int Iframes=0, Pframes=0, use_assembler=0;
	double framepsnr[ABS_MAXFRAMENR];
  
	double Ipsnr=0.,Imaxpsnr=0.,Iminpsnr=999.,Ivarpsnr=0.;
	double Ppsnr=0.,Pmaxpsnr=0.,Pminpsnr=999.,Pvarpsnr=0.;
	double Bpsnr=0.,Bmaxpsnr=0.,Bminpsnr=999.,Bvarpsnr=0.;
  
	char filename[256];
  
	FILE *filehandle;
	FILE *in_file = stdin;

	printf("xvid_stat - XviD core library test program ");
	printf("written by Christoph Lampert 2002\n\n");

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
		else if (strcmp("-d", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_SAVEDECOUTPUT = atoi(argv[i]);
		}
		else if (strcmp("-m", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_SAVEMPEGSTREAM = atoi(argv[i]);
		}
		else if (strcmp("-h", argv[i]) == 0 || strcmp("-help", argv[i])) {
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
		fprintf(stderr,
				"Trying to retreive width and height from PGM header\n",
				XDIM,
				YDIM);
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
		fprintf(stderr,"Wrong Fraterate %s \n",argv[5]);
		return -1;
	}

	if ( ARG_MAXFRAMENR <= 0) {
		fprintf(stderr,"Wrong number of frames\n");
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
	divx_buffer = (unsigned char *) malloc(IMAGE_SIZE(XDIM,YDIM)*2);
	if (!divx_buffer)
		goto free_all_memory;	
    
	out_buffer = (unsigned char *) malloc(IMAGE_SIZE(XDIM,YDIM)*4);	
	if (!out_buffer)
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

	status = dec_init(use_assembler);
	if (status) 
	{
		fprintf(stderr, "Decore INIT problem, return value %d\n", status);
		goto release_all;
	}


/*****************************************************************************
 *                            Main loop
 ****************************************************************************/

	do {

		if (ARG_INPUTTYPE) 
			status = read_pgmdata(in_file, in_buffer);	// read PGM data (YUV-format)
		else
			status = read_yuvdata(in_file, in_buffer);	// read raw data (YUV-format)
	      
		if (status)
		{
			/* Couldn't read image, most likely end-of-file */
			continue;
		}


		if (save_ref_flag)
		{
			sprintf(filename, "%s%05d.pgm", filepath, filenr);
			write_pgm(filename,in_buffer);
		}


/*****************************************************************************
 *                       Analyse this frame before encoding
 ****************************************************************************/

/*
 *	nothing is done here at the moment, but you could e.g. create
 *	histograms or measure entropy or apply preprocessing filters... 
 */

/*****************************************************************************
 *                       Encode and decode this frame
 ****************************************************************************/

		enctime = msecond();
		status = enc_main(in_buffer, divx_buffer, &m4v_size, &frame_type[filenr]);
		enctime = msecond() - enctime;

		totalenctime += enctime;
		totalsize += m4v_size;

		printf("Frame %5d: intra %1d, enctime=%6.1f ms, size=%6d bytes ",
			   (int)filenr, (int)frame_type[filenr], (float)enctime, (int)m4v_size);

		if (ARG_SAVEMPEGSTREAM)
		{
			sprintf(filename, "%sframe%05d.m4v", filepath, filenr);
			filehandle = fopen(filename, "wb");
			fwrite(divx_buffer, m4v_size, 1, filehandle);
			fclose(filehandle);
		}

		dectime = msecond();
		status = dec_main(divx_buffer, out_buffer, m4v_size);
		dectime = msecond() - dectime;
      
		totaldectime += dectime;      
      
      
/*****************************************************************************
 *             Analyse the decoded frame and compare to original
 ****************************************************************************/
      
		framepsnr[filenr] = PSNR(XDIM,YDIM*3/2, in_buffer, XDIM, out_buffer, XDIM);
	
		printf("dectime =%6.1f ms PSNR %5.2f\n",dectime, framepsnr[filenr]);

		if (ARG_SAVEDECOUTPUT)
		{
			sprintf(filename, "%sdec%05d.pgm", filepath, filenr);
			write_pgm(filename, out_buffer);
		}

		/* Read the header if it's pgm stream */ 
		if (ARG_INPUTTYPE)
			status = read_pgmheader(in_file);

		filenr++;

	} while ( (!status) && (filenr<ARG_MAXFRAMENR) );

	
      
/*****************************************************************************
 *         Calculate totals and averages for output, print results
 ****************************************************************************/

	totalsize    /= filenr;
	totalenctime /= filenr;
	totaldectime /= filenr;

	for (i=0;i<filenr;i++)
	{
		switch (frame_type[i])
		{
		case 0:
			Pframes++;
			Ppsnr += framepsnr[i];
			break;
		case 1:
			Iframes++;
			Ipsnr += framepsnr[i];
			break;
		default:
			break;
		}
	}
	
	if (Pframes)
		Ppsnr /= Pframes;	
	if (Iframes)
		Ipsnr /= Iframes;	

	/* calculate statistics for every frametype: P,I */
	for (i=0;i<filenr;i++)
	{	
		switch (frame_type[i])
		{
		case 0:
			if (framepsnr[i] > Pmaxpsnr)
				Pmaxpsnr = framepsnr[i];
			if (framepsnr[i] < Pminpsnr)
				Pminpsnr = framepsnr[i];
			Pvarpsnr += (framepsnr[i] - Ppsnr)*(framepsnr[i] - Ppsnr) /Pframes;
			break;
		case 1:
			if (framepsnr[i] > Imaxpsnr)
				Imaxpsnr = framepsnr[i];
			if (framepsnr[i] < Pminpsnr)
				Iminpsnr = framepsnr[i];
			Ivarpsnr += (framepsnr[i] - Ipsnr)*(framepsnr[i] - Ipsnr) /Iframes;
		default:
			break;
		}
	}
	
	/* Print all statistics */
	printf("Avg. Q%1d %2s ",ARG_QUALITY, (ARG_QUANTI ? " q" : "br"));
	printf("%04d ",MAX(ARG_QUANTI,ARG_BITRATE));
	printf("( %.2f bpp) ", (double)ARG_BITRATE*1000/XDIM/YDIM/ARG_FRAMERATE);
	printf("size %6d ",totalsize);
	printf("( %4d kbps ",(int)(totalsize*8*ARG_FRAMERATE/1000));
	printf("/ %.2f bpp) ",(double)totalsize*8/XDIM/YDIM);
	printf("enc: %6.1f fps, dec: %6.1f fps \n",1/totalenctime, 1/totaldectime);
	printf("PSNR P(%d): %5.2f ( %5.2f , %5.2f ; %5.4f ) ",Pframes,Ppsnr,Pminpsnr,Pmaxpsnr,sqrt(Pvarpsnr/filenr));
	printf("I(%d): %5.2f ( %5.2f , %5.2f ; %5.4f ) ",Iframes,Ipsnr,Iminpsnr,Imaxpsnr,sqrt(Ivarpsnr/filenr));
	printf("\n");

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

	if (dec_handle)
	{
		status = dec_stop();
		if (status)    
			fprintf(stderr, "Decore RELEASE problem return value %d\n", status);
	}

	fclose(in_file);

 free_all_memory:
	free(out_buffer);
	free(divx_buffer);
	free(in_buffer);

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
	clock_t clk;
	   
	clk = clock();
			
	return clk * 1000 / CLOCKS_PER_SEC;

}


/* 
 * Returns the sum of squared distances (SSD) between two images of dimensions
 * x times y
 */
static double absdistq(int x, int y,
					   unsigned char* buf1, int stride1,
					   unsigned char* buf2, int stride2)
{
	double dist=0.;
	int i,j,val;

	for (i=0;i<y;i++)
	{	
		val=0;
		for (j=0;j<x;j++)
			val+= ((int)buf1[j]-(int)buf2[j])*((int)buf1[j]-(int)buf2[j]);
	
		dist += (double)val;
		buf1 += stride1;
		buf2 += stride2;
	}
	return dist/(x*y);
}


/*
 * Returns the PSNR between to images.
 *
 * This is a common logarithmic measure for "quality" from the world of signal
 * processing if you don't know what it is, simply accept that higher values
 * are better.
 *
 * PSNR represents the ratio of useful signal over noise signal. In our case,
 * useful signal is refernce image, noise signal is the difference between
 * reference and decoded frame from encoded bitstream.
 *
 * The problem is this type of value is dependant of image source and so, is
 * not reliable as a common "quality" indicator.
 * So PSNR computes the ratio of maximum/noise. Maximum being set to 2^bpp/channel
 * This way, PSNR is not dependant anymore of image data type.
 *
 */
static double PSNR(int x, int y,
				   unsigned char* buf1, int stride1,
				   unsigned char* buf2, int stride2)
{
	return 10*(log10(255*255)-log10( absdistq(x, y, buf1, stride1, buf2, stride2) ));
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
	fprintf(stderr, " -f float       : target framerate (>0)\n");
	fprintf(stderr, " -i string      : input filename (default=stdin)\n");
	fprintf(stderr, " -t integer     : input data type (yuv=0, pgm=1)\n");
	fprintf(stderr, " -n integer     : number of frames to encode\n");
	fprintf(stderr, " -q integer     : quality ([0..5])\n");
	fprintf(stderr, " -d boolean     : save decoder output (0 False*, !=0 True)\n");
	fprintf(stderr, " -m boolean     : save mpeg4 raw stream (0 False*, !=0 True)\n");
	fprintf(stderr, " -h, -help      : prints this help message\n");
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
   
	if (fread(image, 1, IMAGE_SIZE(XDIM, YDIM), handle) != IMAGE_SIZE(XDIM, YDIM)) 
		return 1;
	else	
		return 0;
}

static int write_pgm(char *filename, unsigned char *image)
{
	int loop;

	unsigned char *y = image;
	unsigned char *u = image + XDIM*YDIM;
	unsigned char *v = image + XDIM*YDIM + XDIM/2*YDIM/2;

	FILE *filehandle;
	filehandle=fopen(filename,"w+b");
	if (filehandle)
	{
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

#ifdef ARCH_IA64
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
					int *streamlength, int* frametype)
{
	int xerr;

	XVID_ENC_FRAME xframe;
	XVID_ENC_STATS xstats;

	xframe.bitstream = bitstream;
	xframe.length = -1; 	// this is written by the routine

	xframe.image = image;
	xframe.colorspace = XVID_CSP_YV12;	// defined in <xvid.h>

	xframe.intra = -1; // let the codec decide between I-frame (1) and P-frame (0)

	xframe.quant = ARG_QUANTI;	// is quant != 0, use a fixed quant (and ignore bitrate)

	xframe.motion = motion_presets[ARG_QUALITY];
	xframe.general = general_presets[ARG_QUALITY];
	xframe.quant_intra_matrix = xframe.quant_inter_matrix = NULL;

	xerr = xvid_encore(enc_handle, XVID_ENC_ENCODE, &xframe, &xstats);

	/*
	 * This is statictical data, e.g. for 2-pass. If you are not
	 * interested in any of this, you can use NULL instead of &xstats
	 */
	*frametype = xframe.intra;
	*streamlength = xframe.length;

	return xerr;
}

/*****************************************************************************
 * Routines for decoding: init encoder, frame step, release encoder
 ****************************************************************************/

/* init decoder before first run */
static int dec_init(int use_assembler)
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

/* decode one frame  */
static int dec_main(unsigned char *m4v_buffer, unsigned char *out_buffer,
					int m4v_size)
{
	int xerr;
	XVID_DEC_FRAME xframe;

	xframe.bitstream = m4v_buffer;
	xframe.length = m4v_size;
	xframe.image = out_buffer;
	xframe.stride = XDIM;
	xframe.colorspace = XVID_CSP_YV12;             // XVID_CSP_USER is fastest (no memcopy involved)

	xerr = xvid_decore(dec_handle, XVID_DEC_DECODE, &xframe, NULL);

	return xerr;
}

/* close decoder to release resources */
static int dec_stop()
{
	int xerr;
	xerr = xvid_decore(dec_handle, XVID_DEC_DESTROY, NULL, NULL);

	return xerr;
}

/* EOF */
