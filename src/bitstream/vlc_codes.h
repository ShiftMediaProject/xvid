#ifndef _VLC_CODES_H_
#define _VLC_CODES_H_

#include "../portab.h"

#define VLC_ERROR	(-1)

#define ESCAPE  3
#define ESCAPE1 6
#define ESCAPE2 14
#define ESCAPE3 15

typedef struct
{
	uint32_t code;
	uint8_t len;
}
VLC;

typedef struct
{
	uint8_t last;
	uint8_t run;
	int8_t level;
}
EVENT;

typedef struct
{
	uint8_t len;
	EVENT event;
}
REVERSE_EVENT;

typedef struct
{
	VLC vlc;
	EVENT event;
}
VLC_TABLE;


/******************************************************************
 * common tables between encoder/decoder                          *
 ******************************************************************/

extern short const dc_threshold[];

extern VLC const dc_lum_tab[];
extern VLC_TABLE const coeff_tab[2][102];
extern uint8_t const max_level[2][2][64];
extern uint8_t const max_run[2][2][64];
extern VLC sprite_trajectory_code[32768];
extern VLC sprite_trajectory_len[15];
extern VLC mcbpc_intra_tab[15];
extern VLC mcbpc_inter_tab[29];
extern const VLC cbpy_tab[16];
extern const VLC dcy_tab[511];
extern const VLC dcc_tab[511];
extern const VLC mb_motion_table[65];
extern VLC const mcbpc_intra_table[64];
extern VLC const mcbpc_inter_table[257];
extern VLC const cbpy_table[64];
extern VLC const TMNMVtab0[];
extern VLC const TMNMVtab1[];
extern VLC const TMNMVtab2[];
extern short const dc_threshold[];
extern VLC const dc_lum_tab[];


#endif /* _VLC_CODES_H */
