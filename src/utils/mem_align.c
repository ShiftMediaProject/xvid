#include <stdlib.h>
#include <stdio.h>
#include "mem_align.h"

void *xvid_malloc(size_t size, uint8_t alignment)
{
  uint8_t *mem_ptr;
  
  if(alignment == 0)
  {
	  if((mem_ptr = (uint8_t *) malloc(size + 1)) != NULL) {
		*mem_ptr = 0;
		return (void *) mem_ptr++;
	  }
  }
  else
  {
	  uint8_t *tmp;
	
	  if((tmp = (uint8_t *) malloc(size + alignment)) != NULL) {
		mem_ptr = (uint8_t *) (((uint32_t) tmp / alignment) * alignment + alignment);
		*(mem_ptr - 1) = (alignment - 1) - ((uint32_t) tmp % alignment);
		return (void *) mem_ptr;
	  }
  }

  return NULL;
}

void xvid_free(void *mem_ptr)
{
  uint8_t *tmp;

  tmp = (uint8_t *) mem_ptr - 1;
  mem_ptr  = tmp - *tmp;

  free(mem_ptr);
}
