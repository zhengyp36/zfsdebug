#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <xutils/file_map.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE (sysconf(_SC_PAGESIZE))
#endif

#ifndef MIN
#define MIN(x,y) ((x) <= (y) ? (x) : (y))
#endif

static inline size_t
calc_map_size(size_t size)
{
	return ((size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
}

static inline int
fm_mapped(file_map_t *fm)
{
	return (fm->buf != NULL);
}

static int
do_map(file_map_t *fm, size_t spec_size)
{
	int oflags = fm->read_only ? O_RDONLY /*| O_DIRECT*/: O_RDWR;
	int mflags = fm->read_only ? PROT_READ : (PROT_READ | PROT_WRITE);

	#define SYS_DEV_PATH "/dev/sda"
	#define SYS_DEV_PATH_LEN (sizeof(SYS_DEV_PATH)-1)
	if (!fm->read_only &&
	    !strncmp(fm->path, SYS_DEV_PATH, SYS_DEV_PATH_LEN)) {
		printf("Error: *** It's forbidden to map %s writable\n",
		    SYS_DEV_PATH);
		return (-1);
	}

	int fd = open(fm->path, oflags);
	if (fd < 0) {
		printf("Error: *** open %s error %d\n", fm->path, errno);
		return (-1);
	}

	struct stat st;
	if (fstat(fd, &st)) {
		printf("Error: *** stat %s, fd %d, error %d\n",
		    fm->path, fd, errno);
		close(fd);
		return (-1);
	}

	fm->size = st.st_size ? st.st_size : spec_size;
	fm->spec_size = spec_size;

	if (fm->size == 0)
		fm->size = devsize(fm->path) * 512;

	if (fm->size == 0) {
		printf("Error: *** size of %s is zero\n", fm->path);
		close(fd);
		return (-1);
	}

	fm->buf = mmap(NULL, calc_map_size(fm->size),
	    mflags, MAP_SHARED, fd, 0);
	close(fd);

	if (fm->buf == MAP_FAILED) {
		printf("Error: *** map %s with size %lx error %d\n",
		    fm->path, fm->size, errno);
		fm->buf = NULL;
		return (-1);
	}

	return (0);
}

static inline void
do_unmap(file_map_t *fm)
{
	munmap(fm->buf, calc_map_size(fm->size));
	fm->buf = NULL;
}

static int
touch_file(const char *path)
{
	FILE *fp = fopen(path, "a+");
	if (!fp) {
		printf("Error: *** touch %s error %d\n", path, errno);
		return (-1);
	}
	fclose(fp);
	return (0);
}

static int
get_file_size(const char *path, size_t *size)
{
	struct stat st;
	if (stat(path, &st)) {
		printf("Error: *** stat '%s' errno %d\n", path, errno);
		return (-1);
	}

	*size = st.st_size;
	return (0);
}

static int
expand(const char *path, size_t size)
{
	if (touch_file(path))
		return (-1);

	size_t orig_size;
	if (get_file_size(path, &orig_size))
		return (-1);

	if (size > 0) {
		if (truncate(path, orig_size + size)) {
			printf("Error: *** expand %s size %ld(+%ld) "
			    "errno %d\n", path, orig_size, size, errno);
			return (-1);
		}
	}

	return (0);
}

static int
set_size(const char *path, size_t size, int *changed)
{
	if (touch_file(path))
		return (-1);

	size_t orig_size;
	if (get_file_size(path, &orig_size))
		return (-1);

	*changed = 0;
	if (orig_size != size) {
		if (truncate(path, size)) {
			printf("Error: *** truncate %s size %ld => "
			    "%ld errno %d\n", path, orig_size, size, errno);
			return (-1);
		}
		*changed = 1;
	}

	return (0);
}

static void
file_map_init_impl(file_map_t *fm, const char *path, int read_only)
{
	memset(fm, 0, sizeof(*fm));
	fm->path = strdup(path);
	assert(fm->path);
	fm->read_only = read_only;
}

void
file_map_init(file_map_t *fm, const char *path)
{
	file_map_init_impl(fm, path, 0);
}

void
file_map_init_rd(file_map_t *fm, const char *path)
{
	file_map_init_impl(fm, path, 1);
}

int
file_map_set_size(file_map_t *fm, size_t size)
{
	if (fm->read_only) {
		printf("Error: *** %s is readonly-open\n", fm->path);
		return (-1);
	}

	int changed = 0;
	int rc = set_size(fm->path, size, &changed);
	if (!rc && changed && fm_mapped(fm)) {
		do_unmap(fm);
		rc = do_map(fm, 0);
	}
	return (rc);
}

int
file_map_expand(file_map_t *fm, size_t size)
{
	if (fm->read_only) {
		printf("Error: *** %s is readonly-open\n", fm->path);
		return (-1);
	}

	int rc = expand(fm->path, size);
	if (!rc && fm_mapped(fm)) {
		do_unmap(fm);
		rc = do_map(fm, 0);
	}
	return (rc);
}

int
file_map_open_(file_map_t *fm, size_t spec_size)
{
	if (fm_mapped(fm))
		return (0);

	if (access(fm->path, F_OK))
		if (fm->read_only || expand(fm->path, PAGE_SIZE))
			return (-1);
	return (do_map(fm, spec_size));
}

int
file_map_reopen(file_map_t *fm, int read_only)
{
	if (!fm_mapped(fm)) {
		printf("Error: *** file(%s) is not opened\n", fm->path);
		return (-1);
	}

	fm->read_only = read_only;
	do_unmap(fm);
	return (do_map(fm, fm->spec_size));
}

void
file_map_fini(file_map_t *fm)
{
	if (fm_mapped(fm))
		do_unmap(fm);
	free((void*)fm->path);
}

size_t
devsize(const char *dev)
{
	if (strncmp(dev, "/dev/", strlen("/dev/")) != 0)
		return (0);
	else if (strchr(dev + strlen("/dev/"), '/') != NULL)
		return (0);

	/* skip prefix '/dev/' and get devname */
	const char *devname = dev + strlen("/dev/");

	/* remove partition-id at the tail of devname */
	int namelen = strlen(devname);
	while (namelen > 0) {
		char ch = devname[namelen - 1];
		if (ch >= '0' && ch <= '9')
			namelen--;
		else
			break;
	}

	/* namelen should not be too long */
	#define MAX_NAME_LEN 16
	if (namelen == 0 || namelen > MAX_NAME_LEN)
		return (0);

	char path[128];
	if (devname[namelen] != '\0') {
		int id = 0;
		for (const char *s = devname + namelen; *s; s++)
			id = id * 10 + (*s - '0');

		char name[MAX_NAME_LEN + 1];
		memcpy(name, devname, namelen);
		name[namelen] = '\0';

		snprintf(path, sizeof(path), "/sys/block/%s/%s%u/size",
		    name, name, id);
	} else {
		snprintf(path, sizeof(path), "/sys/block/%s/size", devname);
	}

	size_t size = 0;
	FILE *fp = fopen(path, "rb");
	if (fp) {
		if (fscanf(fp, "%ld", &size) != 1)
			size = 0;
		fclose(fp);
	}

	return (size);
}
