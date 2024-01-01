#ifndef _XUTILS_FILE_MAP_H
#define _XUTILS_FILE_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct file_map {
	const char *	path;
	void *		buf;
	size_t		size;
	size_t		spec_size;
	int		read_only;
} file_map_t;

size_t devsize(const char *dev);
void file_map_init(file_map_t *fm, const char *path);
void file_map_init_rd(file_map_t *fm, const char *path);
int  file_map_set_size(file_map_t *fm, size_t size);
int  file_map_expand(file_map_t *fm, size_t size);
int  file_map_open_(file_map_t *fm, size_t spec_size);
int  file_map_reopen(file_map_t *fm, int read_only);
void file_map_fini(file_map_t *fm);

#define file_map_open(fm, ...) file_map_open_(fm_open_args(fm, ##__VA_ARGS__))
#define fm_open_args(fm, ...) fm_open_args_(fm, ##__VA_ARGS__, 0)
#define fm_open_args_(fm, spec_size, ...) fm, spec_size

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _XUTILS_FILE_MAP_H
