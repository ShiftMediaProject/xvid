#include <stdio.h>
#include <stdlib.h>

#include "xvid.h"
#include "bitstream.h"		// for BitstreamReadHeaders() to get video width & height

#define	ERR_OK					(0)
#define ERR_NO_OPT				(1)
#define ERR_OPT_NOT_SUPPORT		(2)
#define ERR_FILE				(3)
#define ERR_VERSION				(4)
#define ERR_MEMORY				(5)


#define BUF_LEN					(352*288*3)
#define MAX_FRAME_SIZE			(65535)
