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
 *  PSNR and Speed test routine for XviD using the XviD-API                      
 *  (C) Christoph Lampert, 2002/04/13
 *		                    
 *  A sequence of YUV pics in PGM file format is encoded and decoded
 *  The speed is measured and PSNR of decoded picture is calculated. 
 *		                   
 *  The program is plain C and needs no libraries except for libxvidcore, 
 *  and maths-lib ,so with UN*X you simply compile by
 * 
 *   gcc xvid_stat.c -lxvidcore -lm -o xvid_stat
 *
 *  Run without or with illegal parameters, then PGM input input is read 
 *  from stdin.
 * 
 *  Parameters are: xvid_stat XDIM YDIM QUALITY BITRATE/QUANTIZER FRAMERATE 
 * 
 *  if XDIM or YDIM are illegal (e.g. 0), they are ignored and input is 
 *  considered to be PGM. Otherwise (X and Y both greater than 0) raw YUV 
 *  is expected, as e.g. the standard MPEG test-files, like "foreman"
 * 
 *  0 <= QUALITY <= 6  (default 5)
 *
 *  BITRATE is in kbps (default 900), 
 *      if BITRATE<32, then value is taken is fixed QUANTIZER
 * 
 *  FRAMERATE is a float (with or without decimal dot), default is 25.00
 *
 *  input/output and m4v-output is saved, if corresponding flags are set 
 * 
 *  PGM input must in a very specific format, see read_pgmheader 
 *  it can be generated e.g. from MPEG2 by    mpeg2dec -o pgmpipe   
 *	
 ************************************************************************/

/************************************************************************
 *		                    
 *  For EXAMPLES how to use this, see the seperate file xvid_stat.examples
 *
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>		// needed for log10
#include <sys/time.h>		// only needed for gettimeofday

#include "../src/xvid.h"		/* comes with XviD */

int motion_presets[7] = {
	0,                                                              // Q 0
	PMV_EARLYSTOP16,                                                // Q 1
	PMV_EARLYSTOP16,						// Q 2
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,				// Q 3
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,				// Q 4
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EARLYSTOP8	 	// Q 5
 			| PMV_HALFPELREFINE8, 	
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EXTSEARCH16 	// Q 6
			| PMV_USESQUARES16 | PMV_EARLYSTOP8 | PMV_HALFPELREFINE8
	};

int general_presets[7] = {
	XVID_H263QUANT,	/* or use XVID_MPEGQUANT */		// Q 0
	XVID_MPEGQUANT, 					// Q 1
	XVID_H263QUANT, 			       // Q 2
	XVID_H263QUANT | XVID_HALFPEL,				// Q 3
	XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V,		// Q 4
	XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V,		// Q 5
	XVID_H263QUANT | XVID_HALFPEL | XVID_INTER4V };		// Q 6
		

/* my default values for encoding */

#define ABS_MAXFRAMENR 9999               // max number of frames

int ARG_BITRATE=900;
int ARG_QUANTI=0;

int ARG_QUALITY =6;
int ARG_MINQUANT=1;
int ARG_MAXQUANT=31;
float ARG_FRAMERATE=25.00;

int ARG_MAXFRAMENR=ABS_MAXFRAMENR;


#define MAX(A,B) ( ((A)>(B)) ? (A) : (B) )
#define SMALL_EPS 1e-10


/* these are global variables. Not very elegant, but easy, and this is an easy program */
	
int XDIM=0;
int YDIM=0;	// will be set when reading first image
int i,filenr = 0;

int save_m4v_flag = 0;		// save MPEG4-bytestream?
int save_dec_flag = 1;		// save decompressed bytestream?
int save_ref_flag = 0;		// 

int pgmflag = 0;		// a flag, if input is in PGM format, overwritten in init-phase
char filepath[256] = "./";	// the path where to save output 

void *enc_handle = NULL;		// internal structures (handles) for encoding
void *dec_handle = NULL;		// and decoding


/*********************************************************************/
/*	               "statistical" functions                       */
/*                                                                   */
/*  these are not needed for encoding or decoding, but for measuring */
/*  time and quality, there in nothing specific to XviD in these     */
/*                                                                   */
/*********************************************************************/

double msecond()  	
/* return the current time in seconds(!)  */
{	
	struct timeval  tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec + tv.tv_usec * 1.0e-6;
}



double absdistq(int x,int y, unsigned char* buf1, int stride1, unsigned char* buf2, int stride2)
/* returns the sum of squared distances (SSD) between two images of dimensions x times y */
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


double PSNR(int x,int y, unsigned char* buf1, int stride1, unsigned char* buf2, int stride2 )
/* return the PSNR between to images                                               */
/* this is a logarithmic measure for "quality" from the world of signal processing */
/* if you don't know what it is, simply accept that higher values are better       */
{
   return 10*(log10(255*255)-log10( absdistq(x, y, buf1, stride1, buf2, stride2) ));
}


/*********************************************************************/
/*	              input and output functions                     */
/*                                                                   */
/* the are small and simple routines to read and write PGM and YUV   */
/* image. It's just for convenience, again nothing specific to XviD  */
/*                                                                   */
/*********************************************************************/

int read_pgmheader(FILE* handle)
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
	{	XDIM=xsize;
		YDIM=ysize;
	}

	return 0;
}

int read_pgmdata(FILE* handle, unsigned char *image)
{	
	int i,status;
	char dummy;
   
	unsigned char* buff1_ptr2 = image + XDIM*YDIM;
	unsigned char* buff1_ptr3 = image + XDIM*YDIM + XDIM/2*YDIM/2; 

	fread(image,XDIM*YDIM,1,stdin);	// read Y component of picture
 
   	for (i=0;i<YDIM/2;i++)
	{
		fread(buff1_ptr2,XDIM/2,1,stdin);        // read U
		buff1_ptr2 += XDIM/2;
		fread(buff1_ptr3,XDIM/2,1,stdin);      	 // read V
		buff1_ptr3 += XDIM/2;
	}
	fread(&dummy,1,1,handle);	//  I don't know why, but this seems needed
	return 0;
}

int read_yuvdata(FILE* handle, unsigned char *image)
{	int i;
	char dummy;
   
	unsigned char* buff1_ptr2 = image + XDIM*YDIM;
	unsigned char* buff1_ptr3 = image + XDIM*YDIM + XDIM/2*YDIM/2; 

	if (fread(image,XDIM,YDIM*3/2,stdin) != YDIM*3/2) 
		return 1;
	else	
		return 0;
}

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



/*********************************************************************/
/* Routines for encoding: init encoder, frame step, release encoder  */
/*********************************************************************/

#define FRAMERATE_INCR 1001


int enc_init(int use_assembler)
{	/* initialize encoder for first use, pass all needed parameters to the codec */
	int xerr;
	
	XVID_INIT_PARAM xinit;
	XVID_ENC_PARAM xparam;

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
	xparam.min_quantizer = 1;
	xparam.max_quantizer = 31;
	xparam.max_key_interval = (int)ARG_FRAMERATE*10;

		/* I use a small value here, since will not encode whole movies, but short clips */

	xerr = xvid_encore(NULL, XVID_ENC_CREATE, &xparam, NULL);
	enc_handle=xparam.handle;

	return xerr;
}

int  enc_stop()
{	int xerr;

	xerr = xvid_encore(enc_handle, XVID_ENC_DESTROY, NULL, NULL);
        return xerr;
}

int  enc_main(unsigned char* image, unsigned char* bitstream, int *streamlength, int* frametype)
{	int xerr;

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

/*	        enc_result->is_key_frame = xframe.intra;
	        enc_result->quantizer = xframe.quant;
	        enc_result->total_bits = xframe.length * 8;
	        enc_result->motion_bits = xstats.hlength * 8;
	        enc_result->texture_bits = enc_result->total_bits - enc_result->motion_bits;
*/ 

/*  This is statictical data, e.g. for 2-pass. 
    If you are not interested in any of this, you can use NULL instead of &xstats
*/
	*frametype = xframe.intra;
	*streamlength = xframe.length;

	return xerr;
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

int dec_main(unsigned char *m4v_buffer, unsigned char *out_buffer, int m4v_size)
{	/* decode one frame  */

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
  unsigned char *in_buffer = NULL;
  unsigned char *out_buffer = NULL;

  double enctime,dectime;
  double totalenctime=0.;
  double totaldectime=0.;
  
  long totalsize=0;
  int status;
  
  int m4v_size;
  int frame_type[ABS_MAXFRAMENR];
  int Iframes=0, Pframes=0, Bframes=0, use_assembler=0;
  double framepsnr[ABS_MAXFRAMENR];
  
  double Ipsnr=0.,Imaxpsnr=0.,Iminpsnr=999.,Ivarpsnr=0.;
  double Ppsnr=0.,Pmaxpsnr=0.,Pminpsnr=999.,Pvarpsnr=0.;
  double Bpsnr=0.,Bmaxpsnr=0.,Bminpsnr=999.,Bvarpsnr=0.;
  
  char filename[256];
  
  FILE *filehandle;

/* read YUV in pgm format from stdin */
  if (!pgmflag)
  {	
	pgmflag = 1;

	if (argc==2 && !strcmp(argv[1],"-asm")) 
	  use_assembler = 1;
  	if (argc>=3)
	{	XDIM = atoi(argv[1]);
	  	YDIM = atoi(argv[2]);
		if ( (XDIM <= 0) || (XDIM >= 2048) || (YDIM <=0) || (YDIM >= 2048) )
		{	fprintf(stderr,"Wrong frames size %d %d, trying PGM \n",XDIM, YDIM); 
		}
		else 
		{
			YDIM = YDIM*3/2; /* for YUV */
			pgmflag = 0;
		}
	}
  }
  
  if (pgmflag)  
  {	if (read_pgmheader(stdin))
	    {
	      printf("Wrong input format, I want YUV encapsulated in PGM\n");
	      return 1;
	    }
  }
  if (argc>=4)
  {	ARG_QUALITY = atoi(argv[3]);
  	if ( (ARG_QUALITY < 0) || (ARG_QUALITY > 6) )
		{ fprintf(stderr,"Wrong Quality\n"); return -1; }
	else
		  printf("Quality %d\n",ARG_QUALITY);
  }
  if (argc>=5)
  {	ARG_BITRATE = atoi(argv[4]);
  	if ( (ARG_BITRATE <= 0) )
		{ fprintf(stderr,"Wrong Bitrate\n"); return -1; }
  	if ( (ARG_BITRATE <= 32) )
		{ ARG_QUANTI = ARG_BITRATE; 
		  ARG_BITRATE=0;
		  printf("Quantizer %d\n",ARG_QUANTI);
		}
	else
		  printf("Bitrate %d kbps\n",ARG_BITRATE);
  }
  if (argc>=6)
  {     ARG_FRAMERATE = (float)atof(argv[5]);
        if ( (ARG_FRAMERATE <= 0) )
                { fprintf(stderr,"Wrong Fraterate %s \n",argv[5]); return -1; }
        printf("Framerate %6.3f fps\n",ARG_FRAMERATE);
  } 

  if (argc>=7)
  {	ARG_MAXFRAMENR = atoi(argv[6]);
	if ( (ARG_MAXFRAMENR <= 0) )
	 { fprintf(stderr,"Wrong number of frames\n"); return -1; }
	printf("max. Framenr. %d\n",ARG_MAXFRAMENR);
  }

/* now we know the sizes, so allocate memory */

  in_buffer = (unsigned char *) malloc(XDIM*YDIM);	
  if (!in_buffer)
    goto free_all_memory;	// goto is one of the most underestimated instructions in C !!!

  divx_buffer = (unsigned char *) malloc(XDIM*YDIM*2);	// this should really be enough memory!
  if (!divx_buffer)
    goto free_all_memory;	
    
  YDIM = YDIM*2/3; // PGM is YUV 4:2:0 format, so real image height is *2/3 of PGM picture 
    
  out_buffer = (unsigned char *) malloc(XDIM*YDIM*4);	
  if (!out_buffer)
    goto free_all_memory;	


/*********************************************************************/
/*	                   XviD PART  Start                          */
/*********************************************************************/


  status = enc_init(use_assembler);
	if (status)    
	{ 
		printf("Encore INIT problem, return value %d\n", status);
		goto release_all;
	}

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
	if (pgmflag) 
	      status = read_pgmdata(stdin, in_buffer);	// read PGM data (YUV-format)
	else
	      status = read_yuvdata(stdin, in_buffer);	// read raw data (YUV-format)
	      
      if (status)
	{
	  // Couldn't read image, most likely end-of-file
	  continue;
	}


      if (save_ref_flag)
	{
		sprintf(filename, "%s%05d.pgm", filepath, filenr);
		write_pgm(filename,in_buffer);
 	}


/*********************************************************************/
/*               analyse this frame before encoding                  */
/*********************************************************************/

//	nothing is done here at the moment, but you could e.g. create
//	histograms or measure entropy or apply preprocessing filters... 

/*********************************************************************/
/*               encode and decode this frame                        */
/*********************************************************************/

	enctime = -msecond();
	status = enc_main(in_buffer, divx_buffer, &m4v_size, &frame_type[filenr]);
	enctime += msecond();

	totalenctime += enctime;
	totalsize += m4v_size;

	printf("Frame %5d: intra %d, enctime =%6.1f ms length=%7d bytes ",
		 filenr, frame_type[filenr], enctime*1000, m4v_size);

	if (save_m4v_flag)
	{
		sprintf(filename, "%sframe%05d.m4v", filepath, filenr);
		filehandle = fopen(filename, "wb");
		fwrite(divx_buffer, m4v_size, 1, filehandle);
		fclose(filehandle);
	}

	dectime = -msecond();
	status = dec_main(divx_buffer, out_buffer, m4v_size);
	dectime += msecond();
      
	totaldectime += dectime;      
      
      
/*********************************************************************/
/*        analyse the decoded frame and compare to original          */
/*********************************************************************/
      
	framepsnr[filenr] = PSNR(XDIM,YDIM, in_buffer, XDIM, out_buffer, XDIM );
	
	printf("dectime =%6.1f ms PSNR %5.2f\n",dectime*1000, framepsnr[filenr]);

	if (save_dec_flag)
	{
		sprintf(filename, "%sdec%05d.pgm", filepath, filenr);
 	       	write_pgm(filename,out_buffer);
 	}

	if (pgmflag)
		status = read_pgmheader(stdin);		// because if this was the last PGM, stop now 

	filenr++;

   } while ( (!status) && (filenr<ARG_MAXFRAMENR) );

	
      
/*********************************************************************/
/*     calculate totals and averages for output, print results       */
/*********************************************************************/

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
		case 2:
		default:
			Bframes++;
			Bpsnr += framepsnr[i];
			break;
		}
	}
	
	if (Pframes)
		Ppsnr /= Pframes;	
	if (Iframes)
		Ipsnr /= Iframes;	
	if (Bframes)
		Bpsnr /= Bframes;	
		
		
	for (i=0;i<filenr;i++)	// calculate statistics for every frametype: P,I (and B)
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
			break;
		case 2:
			if (framepsnr[i] > Bmaxpsnr)
				Bmaxpsnr = framepsnr[i];
			if (framepsnr[i] < Pminpsnr)
				Bminpsnr = framepsnr[i];
			Bvarpsnr += (framepsnr[i] - Bpsnr)*(framepsnr[i] - Bpsnr) /Bframes;
			break;
		}
	}
	
	printf("Avg. Q%1d %2s ",ARG_QUALITY, (ARG_QUANTI ? " q" : "br"));
	printf("%04d ",MAX(ARG_QUANTI,ARG_BITRATE));
	printf("( %.2f bpp) ", (double)ARG_BITRATE*1000/XDIM/YDIM/ARG_FRAMERATE);
	printf("size %6d ",totalsize);
	printf("( %4d kbps ",(int)(totalsize*8*ARG_FRAMERATE/1000));
	printf("/ %.2f bpp) ",(double)totalsize*8/XDIM/YDIM);
	printf("enc: %6.1f fps, dec: %6.1f fps \n",1/totalenctime, 1/totaldectime);
	printf("PSNR P(%d): %5.2f ( %5.2f , %5.2f ; %5.4f ) ",Pframes,Ppsnr,Pminpsnr,Pmaxpsnr,sqrt(Pvarpsnr/filenr));
	printf("I(%d): %5.2f ( %5.2f , %5.2f ; %5.4f ) ",Iframes,Ipsnr,Iminpsnr,Imaxpsnr,sqrt(Ivarpsnr/filenr));
	if (Bframes)
		printf("B(%d): %5.2f ( %5.2f , %5.2f ; %5.4f ) ",Bframes,Bpsnr,Bminpsnr,Bmaxpsnr,sqrt(Bvarpsnr/filenr));
	printf("\n");
		
/*********************************************************************/
/*	                   XviD PART  Stop                           */
/*********************************************************************/

release_all:

  	if (enc_handle)
	{	
		status = enc_stop();
		if (status)    
			printf("Encore RELEASE problem return value %d\n", status);
	}

  	if (dec_handle)
	{
	  	status = dec_stop();
		if (status)    
			printf("Decore RELEASE problem return value %d\n", status);
	}


free_all_memory:
	free(out_buffer);
	free(divx_buffer);
	free(in_buffer);

  return 0;
}
