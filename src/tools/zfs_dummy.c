#include <stdio.h>
#include <assert.h>
#include <xutils/file_map.h>
#include "zfs_dummy.h"

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

#define ALIGN_DOWN(n,align) ((n) & ~((align)-1))
#define ALIGN_UP(n,align) ALIGN_DOWN((n)+(align)-1,align)

static int
is_all_zero(void *buf, size_t size)
{
	char *start       = buf;
	char *end         = start + size;
	char *align_start = (char*)ALIGN_UP((unsigned long)start, 8);
	char *align_end   = (char*)ALIGN_DOWN((unsigned long)end, 8);
	char *p           = start;
	int sum           = 0;

	for (; !sum && p < align_start; p++)
		sum |= !!*p;
	for (; !sum && p < align_end; p += sizeof(uint64_t))
		sum |= !!*(uint64_t*)p;
	for (; !sum && p < end; p++)
		sum |= !!*p;

	return (!sum);
}

static void *
get_label(file_map_t *fm, int label_ndx)
{
	#define LABEL_CNT 4
	assert(label_ndx >= 0 && label_ndx < LABEL_CNT);
	assert(fm->size > sizeof(vdev_label_t) * LABEL_CNT);

	vdev_label_t *labels = fm->buf;
	char *addr = (void*)&labels[label_ndx];
	long bias = label_ndx < (LABEL_CNT/2) ? 0 :
	    fm->size - sizeof(vdev_label_t) * LABEL_CNT;

	return (addr + bias);
}

void
zfs_dummy_dump_lable_nvlist(file_map_t *fm)
{
	printf("INFO: dump vdev\n");
	printf("\tname    : %s\n", fm->path);
	printf("\tsize    : %ld/0x%lx, %gGB\n",
	    fm->size, fm->size, (double)fm->size / (1024 * 1024 * 1024));
	printf("\tlabel   : %gKB\n", (double)sizeof(vdev_label_t) / 1024);
	for (int i = 0; i < LABEL_CNT; i++) {
		void *label = get_label(fm,i);
		size_t size = sizeof(vdev_label_t);
		int zero = is_all_zero(label, size);
		printf("\tlabel[%d]: %s, {%p,%lx}\n",
		    i, zero ? "zero" : "non-zero", label, size);
	}
}
