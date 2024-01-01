#ifndef _ZFS_DUMMY_H
#define _ZFS_DUMMY_H

#include <stdint.h>

#define	VDEV_PAD_SIZE		(8 << 10)
/* 2 padding areas (vl_pad1 and vl_be) to skip */
#define	VDEV_SKIP_SIZE		VDEV_PAD_SIZE * 2
#define	VDEV_PHYS_SIZE		(112 << 10)
#define	VDEV_UBERBLOCK_RING	(128 << 10)
#define	ZEC_MAGIC		0x210da7ab10c7a11ULL

typedef struct zio_cksum {
	uint64_t	zc_word[4];
} zio_cksum_t;

typedef struct zio_eck {
	uint64_t	zec_magic;	/* for validation, endianness	*/
	zio_cksum_t	zec_cksum;	/* 256-bit checksum		*/
} zio_eck_t;

typedef struct vdev_boot_envblock {
	uint64_t	vbe_version;
	char		vbe_bootenv[VDEV_PAD_SIZE - sizeof (uint64_t) -
			sizeof (zio_eck_t)];
	zio_eck_t	vbe_zbt;
} vdev_boot_envblock_t;

typedef struct vdev_phys {
	char		vp_nvlist[VDEV_PHYS_SIZE - sizeof (zio_eck_t)];
	zio_eck_t	vp_zbt;
} vdev_phys_t;

typedef struct vdev_label {
	char			vl_pad1[VDEV_PAD_SIZE];		/*  8K */
	vdev_boot_envblock_t	vl_be;				/*  8K */
	vdev_phys_t		vl_vdev_phys;			/* 112K */
	char			vl_uberblock[VDEV_UBERBLOCK_RING];/* 128K */
} vdev_label_t;							/* 256K total */

void zfs_dummy_dump_lable_nvlist(struct file_map *);

#endif // _ZFS_DUMMY_H
