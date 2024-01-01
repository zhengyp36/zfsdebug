#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <xutils/file_map.h>
#include "zfs_dummy.h"

/*
 * usage: dumpzfs <dev>
 */

static void
usage(char *appname)
{
    printf("Usage: %s <dev>\n", basename(appname));
}

static int
open_dev(file_map_t *fm, const char *dev)
{
    file_map_init_rd(fm, dev);
    int rc = file_map_open(fm);
    if (rc)
        file_map_fini(fm);
    return (rc);
}

static void
close_dev(file_map_t *fm)
{
    file_map_fini(fm);
}

typedef struct buf {
    void *buf;
    size_t size;
} buf_t;

enum {
    B = 1,
    KB = 1024,
    MB = 1024 * 1024,
    GB = 1024 * 1024 * 1024,
};

static int
get_label(file_map_t *fm, buf_t *buf, int idx)
{
    assert(idx >= 0 && idx < 4);

    if (fm->size < 1 * GB) {
        printf("Error: *** dev(%s) is too small\n", fm->path);
        return (-1);
    }

    buf->size = 256 * KB;
    if (idx < 2)
        buf->buf = fm->buf + buf->size * idx;
    else
        buf->buf = fm->buf + fm->size - buf->size * (4 - idx);

    return (0);
}

static int
buf_equ(buf_t *buf1, buf_t *buf2)
{
    return (buf1->size == buf2->size &&
        memcmp(buf1->buf, buf2->buf, buf1->size));
}

static void
sample_buf(file_map_t *fm, size_t start, size_t end, size_t step, const char *name)
{
    size_t sum = 0;
    if (!end || end > fm->size)
        end = fm->size;

    for (size_t size = start; size < end; size += step)
        sum |= !!*(uint64_t*)(fm->buf + size);

    printf("INFO: range[%ld,%ld]<%s>: %szero.\n",
        start, end, name, sum ? "non-" : "");
}

static void
check_lable(file_map_t *fm)
{
    buf_t buf1, buf2;
    assert(get_label(fm, &buf1, 0) == 0);
    printf("label[0]-info={%p,%lx}\n", buf1.buf, buf1.size);

    assert(get_label(fm, &buf2, 1) == 0);
    assert(buf_equ(&buf1, &buf2));
    printf("label[1]-info={%p,%lx}\n", buf2.buf, buf2.size);

    assert(get_label(fm, &buf2, 2) == 0);
    assert(buf_equ(&buf1, &buf2));
    printf("label[2]-info={%p,%lx}\n", buf2.buf, buf2.size);

    assert(get_label(fm, &buf2, 3) == 0);
    assert(buf_equ(&buf1, &buf2));
    printf("label[3]-info={%p,%lx}\n", buf2.buf, buf2.size);

    printf("INFO: 4 labels equal.\n");
}

static int
dump_zfs(file_map_t *fm)
{
    int rc = 0;

#ifdef FM_SAMPLE_ALL
    sample_buf(fm, 0, 0, 4 * KB, "ALL");
#endif

    check_lable(fm);
    sample_buf(fm, 0, 8192, 8, "BlankSpace:VTOC/EFI");
    sample_buf(fm, 8192, 8192*2, 8, "BootHeader");

    buf_t buf;
    get_label(fm, &buf, 0);
    zfs_dummy_dump_lable_nvlist(fm);

    return (rc);
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        usage(argv[0]);
        return (0);
    }

    if (argc > 2) {
        printf("Error: *** to many arguments.\n");
        usage(argv[0]);
        return (-1);
    }

    file_map_t tmpfm, *fm = &tmpfm;
    if (open_dev(fm, argv[1]))
        return (-1);

    int rc = dump_zfs(fm);

    close_dev(fm);
    return (rc);
}
