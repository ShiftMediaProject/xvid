#ifndef _XVID_FONT_H_
#define _XVID_FONT_H_

#include "image.h"

void image_printf(IMAGE * img, int edged_width, int height, int x, int y, char *fmt, ...);

#endif /* _XVID_FONT_H_ */