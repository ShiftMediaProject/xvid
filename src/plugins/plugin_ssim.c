/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - SSIM plugin: computes the SSIM metric  -
 *
 *  Copyright(C) 2005 Johannes Reinhardt <Johannes.Reinhardt@gmx.de>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *
 *
 ****************************************************************************/

#include <malloc.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include "../xvid.h"
#include "plugin_ssim.h"
#include "../utils/emms.h"

/* needed for visualisation of the error map with X
 display.h borrowed from x264
#include "display.h"*/

typedef struct framestat_t framestat_t;

struct framestat_t{
	int type;
	int quant;
	float ssim_min;
	float ssim_max;
	float ssim_avg;
	framestat_t* next;
};


typedef int (*lumfunc)(uint8_t* ptr, int stride);
typedef void (*csfunc)(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr);

int lum_8x8_mmx(uint8_t* ptr, int stride);
void consim_mmx(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr);
void consim_sse2(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr);

typedef struct{

	plg_ssim_param_t* param;

	/* for error map visualisation
	uint8_t* errmap;
	*/

	/*for average SSIM*/
	float ssim_sum;
	int frame_cnt;

	/*function pointers*/
	lumfunc func8x8;
	lumfunc func2x8;
	csfunc consim;

	/*stats - for debugging*/
	framestat_t* head;
	framestat_t* tail;
} ssim_data_t;

/* append the stats for another frame to the linked list*/
void framestat_append(ssim_data_t* ssim,int type, int quant, float min, float max, float avg){
	framestat_t* act;
	act = (framestat_t*) malloc(sizeof(framestat_t));
	act->type = type;
	act->quant = quant;
	act->ssim_min = min;
	act->ssim_max = max;
	act->ssim_avg = avg;
	act->next = NULL;

	if(ssim->head == NULL){
		ssim->head = act;
		ssim->tail = act;
	} else {
		ssim->tail->next = act;
		ssim->tail = act;
	}
}

/* destroy the whole list*/
void framestat_free(framestat_t* stat){
	if(stat != NULL){
		if(stat->next != NULL) framestat_free(stat->next);
		free(stat);	
	}
	return;
}

/*writeout the collected stats*/
void framestat_write(ssim_data_t* ssim, char* path){
	FILE* out = fopen(path,"w");
	if(out==NULL) printf("Cannot open %s in plugin_ssim\n",path);
	framestat_t* tmp = ssim->head;

	fprintf(out,"SSIM Error Metric\n");
	fprintf(out,"quant   avg     min     max");
	while(tmp->next->next != NULL){
		fprintf(out,"%3d     %1.4f   %1.4f   %1.4f\n",tmp->quant,tmp->ssim_avg,tmp->ssim_min,tmp->ssim_max);
		tmp = tmp->next;
	}
	fclose(out);
}

/*writeout the collected stats in octave readable format*/
void framestat_write_oct(ssim_data_t* ssim, char* path){

	FILE* out = fopen(path,"w");
	if(out==NULL) printf("Cannot open %s in plugin_ssim\n",path);
	framestat_t* tmp;

	fprintf(out,"quant = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		fprintf(out,"%d, ",tmp->quant);
		tmp = tmp->next;
	}
	fprintf(out,"%d];\n\n",tmp->quant);
	
	fprintf(out,"ssim_min = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		fprintf(out,"%f, ",tmp->ssim_min);
		tmp = tmp->next;
	}
	fprintf(out,"%f];\n\n",tmp->ssim_min);

	fprintf(out,"ssim_max = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		fprintf(out,"%f, ",tmp->ssim_max);
		tmp = tmp->next;
	}
	fprintf(out,"%f];\n\n",tmp->ssim_max);

	fprintf(out,"ssim_avg = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		fprintf(out,"%f, ",tmp->ssim_avg);
		tmp = tmp->next;
	}
	fprintf(out,"%f];\n\n",tmp->ssim_avg);

	fprintf(out,"ivop = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		if(tmp->type == XVID_TYPE_IVOP){
			fprintf(out,"%d, ",tmp->quant);
			fprintf(out,"%f, ",tmp->ssim_avg);
			fprintf(out,"%f, ",tmp->ssim_min);
			fprintf(out,"%f; ",tmp->ssim_max);
		}
		tmp = tmp->next;
	}
	fprintf(out,"%d, ",tmp->quant);
	fprintf(out,"%f, ",tmp->ssim_avg);
	fprintf(out,"%f, ",tmp->ssim_min);
	fprintf(out,"%f];\n\n",tmp->ssim_max);

	fprintf(out,"pvop = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		if(tmp->type == XVID_TYPE_PVOP){
			fprintf(out,"%d, ",tmp->quant);
			fprintf(out,"%f, ",tmp->ssim_avg);
			fprintf(out,"%f, ",tmp->ssim_min);
			fprintf(out,"%f; ",tmp->ssim_max);
		}
		tmp = tmp->next;
	}
	fprintf(out,"%d, ",tmp->quant);
	fprintf(out,"%f, ",tmp->ssim_avg);
	fprintf(out,"%f, ",tmp->ssim_min);
	fprintf(out,"%f];\n\n",tmp->ssim_max);

	fprintf(out,"bvop = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		if(tmp->type == XVID_TYPE_BVOP){
			fprintf(out,"%d, ",tmp->quant);
			fprintf(out,"%f, ",tmp->ssim_avg);
			fprintf(out,"%f, ",tmp->ssim_min);
			fprintf(out,"%f; ",tmp->ssim_max);
		}
		tmp = tmp->next;
	}
	fprintf(out,"%d, ",tmp->quant);
	fprintf(out,"%f, ",tmp->ssim_avg);
	fprintf(out,"%f, ",tmp->ssim_min);
	fprintf(out,"%f];\n\n",tmp->ssim_max);

	fclose(out);
}

/*calculate the luminance of a 8x8 block*/
int lum_8x8_c(uint8_t* ptr, int stride){
	int mean=0,i,j;
	for(i=0;i< 8;i++)
		for(j=0;j< 8;j++){
			mean += ptr[i*stride + j];
		}
	return mean;
}

/*calculate the difference between two blocks next to each other on a row*/
int lum_2x8_c(uint8_t* ptr, int stride){
	int mean=0,i;
	/*Luminance*/
	for(i=0;i< 8;i++){
		mean -= *(ptr-1);
		mean += *(ptr+ 8 - 1);
		ptr+=stride;
	}
	return mean;
}

/*calculate contrast and correlation of the two blocks*/
void iconsim_c(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr){
	int valo, valc, devo =0, devc=0, corr=0;
	int i,j;
	for(i=0;i< 8;i++){
		for(j=0;j< 8;j++){
			valo = *ptro - lumo;
			valc = *ptrc - lumc;
			devo += valo*valo;
			ptro++;
			devc += valc*valc;
			ptrc++;
			corr += valo*valc;
		}
	ptro += stride -8;
	ptrc += stride -8;
	}
	*pdevo = devo;
	*pdevc = devc;
	*pcorr = corr;
};

/*calculate the final ssim value*/
static float calc_ssim(int meano, int meanc, int devo, int devc, int corr){
	static const float c1 = (0.01*255)*(0.01*255);
	static const float c2 = (0.03*255)*(0.03*255);
	float fmeano,fmeanc,fdevo,fdevc,fcorr;
	fmeanc = (float) meanc;
	fmeano = (float) meano;
	fdevo = (float) devo;
	fdevc = (float) devc;
	fcorr = (float) corr;
//	printf("meano: %f meanc: %f devo: %f devc: %f corr: %f\n",fmeano,fmeanc,fdevo,fdevc,fcorr);
	return ((2.0*fmeano*fmeanc + c1)*(fcorr/32.0 + c2))/((fmeano*fmeano + fmeanc*fmeanc + c1)*(fdevc/64.0 + fdevo/64.0 + c2));
}

static void ssim_after(xvid_plg_data_t* data, ssim_data_t* ssim){
	int i,j,c=0;
	int width,height,str,ovr;
	unsigned char * ptr1,*ptr2;
	float isum=0, min=1.00,max=0.00, val;
	int meanc, meano;
	int devc, devo, corr;

	#define GRID 1

	width = data->width - 8;
	height = data->height - 8;
	str = data->original.stride[0];
	if(str != data->current.stride[0]) printf("WARNING: Different strides in plugin_ssim original: %d current: %d\n",str,data->current.stride[0]);
	ovr = str - width + (width % GRID);

	ptr1 = (unsigned char*) data->original.plane[0];
	ptr2 = (unsigned char*) data->current.plane[0];



	/*TODO: Thread*/
	for(i=0;i<height;i+=GRID){
		/*begin of each row*/
		meano = meanc = devc = devo = corr = 0;
		meano = ssim->func8x8(ptr1,str);
		meanc = ssim->func8x8(ptr2,str);
		ssim->consim(ptr1,ptr2,str,meano>>6,meanc>>6,&devo,&devc,&corr);
		
		val = calc_ssim(meano,meanc,devo,devc,corr);
		isum += val;
		c++;
		/* for visualisation
		if(ssim->param->b_visualize)
			ssim->errmap[i*width] = (uint8_t) 127*val;
		*/

		if(val < min) min = val;
		if(val > max) max = val;
		ptr1+=GRID;
		ptr2+=GRID;
		/*rest of each row*/
		for(j=1;j<width;j+=GRID){
			/* for grid = 1 use
			meano += ssim->func2x8(ptr1,str);
			meanc += ssim->func2x8(ptr2,str);
			*/			
			meano = ssim->func8x8(ptr1,str);
			meanc = ssim->func8x8(ptr2,str);
			ssim->consim(ptr1,ptr2,str,meano>>6,meanc>>6,&devo,&devc,&corr);
			val = calc_ssim(meano,meanc,devo,devc,corr);
			isum += val;
			c++;
			/* for visualisation
			if(ssim->param->b_visualize)
				ssim->errmap[i*width +j] = (uint8_t) 255*val;
			*/
			if(val < min) min = val;
			if(val > max) max = val;
			ptr1+=GRID;
			ptr2+=GRID;
		}
		ptr1 +=ovr;
		ptr2 +=ovr;
 	}
	isum/=c;
	ssim->ssim_sum += isum;
	ssim->frame_cnt++;

	if(ssim->param->stat_path != NULL)
		framestat_append(ssim,data->type,data->quant,min,max,isum);

/* for visualization
	if(ssim->param->b_visualize){
		disp_gray(0,ssim->errmap,width,height,width, "Error-Map");
		disp_gray(1,data->original.plane[0],data->width,data->height,data->original.stride[0],"Original");
		disp_gray(2,data->current.plane[0],data->width,data->height,data->original.stride[0],"Compressed");
		disp_sync();
	}
*/
	if(ssim->param->b_printstat){
		printf("SSIM: avg: %f min: %f max: %f\n",isum,min,max);
	}

}

static int ssim_create(xvid_plg_create_t* create, void** handle){
	ssim_data_t* ssim;
	plg_ssim_param_t* param;
	int cpu_flags;
	param = (plg_ssim_param_t*) malloc(sizeof(plg_ssim_param_t));
	*param = *((plg_ssim_param_t*) create->param);
	ssim = (ssim_data_t*) malloc(sizeof(ssim_data_t));

	cpu_flags = check_cpu_features();

	ssim->func8x8 = lum_8x8_c;
	ssim->func2x8 = lum_2x8_c;
	ssim->consim = iconsim_c;

	ssim->param = param;

	if(cpu_flags & XVID_CPU_MMX){
		ssim->func8x8 = lum_8x8_mmx;
		ssim->consim = consim_mmx;
	}
	if(cpu_flags & XVID_CPU_SSE2){
		ssim->consim = consim_sse2;
	}

	ssim->ssim_sum = 0.0;
	ssim->frame_cnt = 0;

/* for visualization
	if(param->b_visualize){
		//error map
		ssim->errmap = (uint8_t*) malloc(sizeof(uint8_t)*(create->width-8)*(create->height-8));
	} else {
		ssim->errmap = NULL;
	};
*/

	/*stats*/
	ssim->head=NULL;
	ssim->tail=NULL;

	*(handle) = (void*) ssim;

	return 0;
}	

int xvid_plugin_ssim(void * handle, int opt, void * param1, void * param2){
	ssim_data_t* ssim;
	switch(opt){
		case(XVID_PLG_INFO):
 			((xvid_plg_info_t*) param1)->flags = XVID_REQORIGINAL;
			break;
		case(XVID_PLG_CREATE):
			ssim_create((xvid_plg_create_t*) param1,(void**) param2);
			break;
		case(XVID_PLG_BEFORE):
		case(XVID_PLG_FRAME):
			break;
		case(XVID_PLG_AFTER):
			ssim_after((xvid_plg_data_t*) param1, (ssim_data_t*) handle);
			break;
		case(XVID_PLG_DESTROY):
			ssim = (ssim_data_t*) handle;
			printf("Average SSIM: %f\n",ssim->ssim_sum/ssim->frame_cnt);
			if(ssim->param->stat_path != NULL)
				framestat_write(ssim,ssim->param->stat_path);
			framestat_free(ssim->head);
			//free(ssim->errmap);
			free(ssim->param);	
			free(ssim);
			break;
		default:
			break;
	}
	return 0;
};
