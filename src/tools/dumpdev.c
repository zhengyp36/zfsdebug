#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <xutils/file_map.h>

/*
 * usage: dumpdev <dev> [off] [size]
 */

static void
usage(char *appname)
{
    printf("Usage: %s <dev> [off] [size]\n", basename(appname));
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

static int
search_no_zero(file_map_t *fm)
{
    size_t off;
    uint64_t *data = NULL;

    printf("search non-zero: %s, size=%ld, ...", fm->path, fm->size);
    fflush(stdout);

    for (off = 0; off + sizeof(uint64_t) < fm->size; off += sizeof(uint64_t))
        if (*(data = (uint64_t*)(fm->buf + off)))
            break;

    if (!data || !*data)
        printf(" => All zeros\n");
    else
        printf(" => First non-zero: [0x%lx]=0x%lx\n",
            ((char*)data - (char*)fm->buf), *data);

    return (0);
}

typedef struct {
    file_map_t *fm;
    size_t start, curr, end;
    size_t cnt, cnt_per_line;
    int done;
} dump_ctx_t;

static void
dump_ctx_init(dump_ctx_t *ctx, file_map_t *fm, size_t off, size_t size)
{
    ctx->fm = fm;

    ctx->start = ctx->curr = off & ~7;
    ctx->end = (off + size + 7) & ~7;
    if (ctx->end > fm->size)
        ctx->end = fm->size;

    ctx->cnt = 0;
    ctx->cnt_per_line = 4;
    ctx->done = 0;
}

static int
dump_single_u64(dump_ctx_t *ctx)
{
    if (ctx->curr + sizeof(uint64_t) >= ctx->end) {
        if (!ctx->done) {
            if (ctx->cnt && ctx->cnt % ctx->cnt_per_line != 0)
                printf("\n");
            else
                printf("No data dumped\n");
            ctx->done = 1;
        }
        return (0);
    }

    if (ctx->cnt % ctx->cnt_per_line == 0)
        printf("%016lx|", ctx->curr);

    printf(" %016lx", *(uint64_t*)(ctx->fm->buf + ctx->curr));

    ctx->cnt++;
    ctx->curr += sizeof(uint64_t);

    if (ctx->cnt % ctx->cnt_per_line == 0)
        printf("\n");

    return (1);
}

static int
dump_no_zero_u64(dump_ctx_t *ctx)
{
    if (ctx->curr + sizeof(uint64_t) >= ctx->end) {
        if (!ctx->done) {
            if (ctx->cnt && ctx->cnt % ctx->cnt_per_line != 0)
                printf("\n");
            else
                printf("No data dumped\n");
            ctx->done = 1;
        }
        return (0);
    }

    uint64_t *data = (uint64_t*)(ctx->fm->buf + ctx->curr);
    if (*data) {
        if (ctx->cnt % ctx->cnt_per_line == 0)
            printf("%016lx|", ctx->curr);
        printf(" %016lx", *data);
        ctx->cnt++;
        if (ctx->cnt % ctx->cnt_per_line == 0)
            printf("\n");
    }

    ctx->curr += sizeof(uint64_t);
    return (1);
}

static void
dump_char(uint64_t data)
{
    printf(" ");
    char *array = (char*)&data;
    for (int i = 0; i < sizeof(data)/sizeof(array[0]); i++) {
        if (isprint(array[i]))
            printf("%c", array[i]);
        else
            printf(".");
    }
}

static int
dump_string(dump_ctx_t *ctx)
{
    if (ctx->curr + sizeof(uint64_t) >= ctx->end) {
        if (!ctx->done) {
            if (ctx->cnt && ctx->cnt % ctx->cnt_per_line != 0)
                printf("\n");
            else
                printf("No data dumped\n");
            ctx->done = 1;
        }
        return (0);
    }

    uint64_t *data = (uint64_t*)(ctx->fm->buf + ctx->curr);
    if (*data) {
        if (ctx->cnt % ctx->cnt_per_line == 0)
            printf("%016lx|", ctx->curr);
        dump_char(*data);
        ctx->cnt++;
        if (ctx->cnt % ctx->cnt_per_line == 0)
            printf("\n");
    }

    ctx->curr += sizeof(uint64_t);
    return (1);
}

enum {
    DUMP_ALL = 0,
    DUMP_NO_ZERO,
    DUMP_STRING,
};

typedef int (*handler_t)(dump_ctx_t*);

#define strequ(str,key) (!strcmp(str,key))

static handler_t
get_handler(void)
{
    const char *fmt = getenv("DUMP_FORMAT");
    handler_t handler = dump_single_u64;
    if (fmt) {
        if (strequ(fmt, "NO-ZERO"))
            handler = dump_no_zero_u64;
        else if (strequ(fmt, "STRING"))
            handler = dump_string;
    }
    return (handler);
}

static int
dump_dev(file_map_t *fm, const char *str_off, const char *str_size)
{
    dump_ctx_t ctx;
    dump_ctx_init(&ctx, fm, atol(str_off), atol(str_size));

    handler_t handler = get_handler();

    printf("DUMP: start=%ld/0x%lx, end=%ld/0x%lx, ...\n",
        ctx.start, ctx.start, ctx.end, ctx.end);
    while (handler(&ctx))
        ;

    return (0);
}

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        usage(argv[0]);
        return (0);
    }

    if (argc > 4) {
        printf("Error: *** to many arguments.\n");
        usage(argv[0]);
        return (-1);
    }

    file_map_t tmpfm, *fm = &tmpfm;
    if (open_dev(fm, argv[1]))
        return (-1);

    int rc;
    if (argc == 2) {
        rc = search_no_zero(fm);
    } else {
        const char *str_off = (argc >= 3) ? argv[2] : "0";
        const char *str_size = (argc >= 4) ? argv[3] : "4096";
        rc = dump_dev(fm, str_off, str_size);
    }

    close_dev(fm);
    return (rc);
}
