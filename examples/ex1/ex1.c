/*******************************************************************************
* This file is a example for how to use the xvid core to compress YUV file
*
* 0.02    11.07.2002    chenm001<chenm001@163.com>
*                       add the decode examples
* 0.01b   28.05.2002    chenm001<chenm001@163.com>
*                       fix a little bug for encode only codec I frame
* 0.01a   27.05.2002    chenm001<chenm001@163.com>
*                       fix a little bug for BFRAMES define locate
* 0.01    23.05.2002    chenm001<chenm001@163.com>
*                       Initialize version only support encode examples
*                       the BFRAME option must be match to core compile option
*******************************************************************************/

#define BFRAMES
#define BFRAMES_DEC
#include "ex1.h"

int Encode(char *, int, int, char *);
int Decode(char *, char *);

int main(int argc, char *argv[])
{
	if (argc<4){
		printf("Usage:\n\t%s Encode  infile  width  heigh  outfile\n",argv[0]);
		printf("\t%s Decode  infile  outfile\n",argv[0]);
		return(ERR_NO_OPT);
	}
	switch(toupper(argv[1][0])){
	case 'E':
		Encode(argv[2], atoi(argv[3]), atoi(argv[4]), argv[5]);
		break;
	case 'D':
		Decode(argv[2], argv[3]);
		break;
	default:
		printf("Error option: %c",argv[1][0]);
		return(ERR_OPT_NOT_SUPPORT);
	}
	return(ERR_OK);
}

/************************************************************
*  Encode Example Start
************************************************************/
char inBuf[BUF_LEN];	// Image Buffer
char outBuf[BUF_LEN];

void set_enc_param(XVID_ENC_PARAM *param)
{
	param->rc_bitrate = 1500*1000;		// Bitrate(in bps)
	param->rc_buffer = 100;
	param->rc_averaging_period = 100;
	param->rc_reaction_delay_factor = 16;

	param->fincr = 1;	// 15 fps
	param->fbase = 15;

	param->min_quantizer = 1;
	param->max_quantizer = 31;
	param->max_key_interval = 100;

#ifdef BFRAMES
	param->max_bframes = 0;		// Disable B-frame
#endif
}

void set_enc_frame(XVID_ENC_FRAME *frame)
{
	// set encode frame param
	frame->general = XVID_HALFPEL;
	frame->general |= XVID_INTER4V;
	frame->motion = PMV_HALFPELDIAMOND8;
	frame->image = inBuf;
	frame->bitstream = outBuf;
	frame->length = BUF_LEN;
	frame->colorspace = XVID_CSP_YV12;	// the test.yuv format is YV12
	frame->quant = 0;	// CBR mode

	frame->general |= XVID_MPEGQUANT;	// Use MPEG quant
	frame->quant_inter_matrix = NULL;	// Use default quant matrix
	frame->quant_intra_matrix = NULL;
}


int Encode(char *in, int width, int height, char *out)
{
	XVID_ENC_PARAM	param;
	XVID_INIT_PARAM init_param;
	XVID_ENC_FRAME	frame;
	XVID_ENC_STATS	stats;
	int num=0;	// Encoded frames
	int temp;

	FILE *fpi=fopen(in,"rb"),
		 *fpo=fopen(out,"wb");
    if (fpi == NULL || fpo==NULL){
		if (fpi)
			fclose(fpi);
		if (fpo)
			fclose(fpo);
		return(ERR_FILE);
	}

	if (height*width*3 > sizeof(inBuf)){
		fclose(fpi);
		fclose(fpo);
		return(ERR_MEMORY);
	}

	// get Xvid core status
	init_param.cpu_flags = 0;
	xvid_init(0, 0, &init_param, NULL);
	// Check API Version is 2.1?
	if (init_param.api_version != ((2<<16)|(1)))
		return(ERR_VERSION);
	

	param.width = width;
	param.height = height;
	set_enc_param(&param);	

	// Init Encode
	temp=xvid_encore(0, XVID_ENC_CREATE, &param, NULL);

	// Encode Frame
	temp=fread(inBuf, 1, width*height*3/2, fpi);	// Read YUV data
	while(temp == width*height*3/2){
		printf("Frames=%d\n",num);
		set_enc_frame(&frame);
		if (!(num%param.max_key_interval))
			frame.intra = 1;	// Encode as I-frame
		else
			frame.intra = 0;	// Encode as P-frame
		xvid_encore(param.handle, XVID_ENC_ENCODE, &frame, &stats);
		fwrite(outBuf, 1, frame.length, fpo);
		temp=fread(inBuf, 1, width*height*3/2, fpi);	// Read next YUV data
		num++;
	}
	
	// Free Encode Core
	if (param.handle)
		xvid_encore(param.handle, XVID_ENC_DESTROY, NULL, NULL);

	fclose(fpi);
	fclose(fpo);
	return(ERR_OK);
}

void set_dec_param(XVID_DEC_PARAM *param)
{
	// set to 0 will auto set width & height from encoded bitstream
	param->height = param->width = 0;
}

int Decode(char *in, char *out)
{
	int				width, height,i=0;
	uint32_t		temp;
	long			readed;
	XVID_DEC_PARAM	param;
	XVID_INIT_PARAM init_param;
	DECODER			*dec;
	XVID_DEC_FRAME	frame;
	Bitstream		bs;

	FILE *fpi=fopen(in,"rb"),
		 *fpo=fopen(out,"wb");
    if (fpi == NULL || fpo==NULL){
		if (fpi)
			fclose(fpi);
		if (fpo)
			fclose(fpo);
		return(ERR_FILE);
	}


	init_param.cpu_flags = 0;
	xvid_init(0, 0, &init_param, NULL);
	// Check API Version is 2.1?
	if (init_param.api_version != ((2<<16)|(1)))
		return(ERR_VERSION);
	
	set_dec_param(&param);

	temp = xvid_decore(0, XVID_DEC_CREATE, &param, NULL);
	dec = param.handle;

	if (dec != NULL){
		// to Get Video width & height
		readed = fread(inBuf, 1, MAX_FRAME_SIZE, fpi);
		BitstreamInit(&bs, inBuf, readed);
		BitstreamReadHeaders(&bs, dec, &temp, &temp, &temp, &temp, &temp);
		width  = dec->width;
		height = dec->height;
		xvid_decore(dec, XVID_DEC_DESTROY, NULL, NULL);

		// recreate new decoder because width & height changed!
		param.width  = width;
		param.height = height;
		temp = xvid_decore(0, XVID_DEC_CREATE, &param, NULL);
		dec = param.handle;
		
		// because START_CODE must be 32bit
		while(readed >= 4){
			printf("Decode Frame %d\n",i++);
			frame.bitstream = inBuf;
			frame.length = readed;
			frame.image = outBuf;
			frame.stride = width;
			frame.colorspace = XVID_CSP_YV12;	// for input video clip color space

			temp = xvid_decore(dec, XVID_DEC_DECODE, &frame, NULL);

			// undo unused byte
			fseek(fpi, frame.length - readed - 2, SEEK_CUR);

			// Write decoded YUV image, size = width * height * 3 / 2 because in I420 CSP
			fwrite(outBuf, 1, width * height * 3 / 2, fpo);

			// read next frame data
			readed = fread(inBuf, 1, MAX_FRAME_SIZE, fpi);
		}

		// free decoder
		xvid_decore(dec, XVID_DEC_DESTROY, NULL, NULL);
	}

	fclose(fpi);
	fclose(fpo);
	return(ERR_OK);
}

