#pragma once

#include <stddef.h>
#include <stdint.h>

/** Max read size for engine input files (see engine_fs_read_entire). */
#define ENGINE_FS_READ_BUF_SIZE (256u * 1024u)

struct xfs_handle_t;
struct xfs_engine_mount_t;

int engine_fs_parse_mount_spec(const char *spec, int *mount_index_out, const char **path_out);

int engine_fs_open_read(int mount_index, const char *path, struct xfs_handle_t *h,
                        struct xfs_engine_mount_t **mnt_out);

void engine_fs_close(struct xfs_engine_mount_t *mnt, struct xfs_handle_t *h);

int engine_fs_ram_mount(struct xfs_engine_mount_t *ram);

int engine_fs_ram_open_write(struct xfs_engine_mount_t *ram, struct xfs_handle_t *h, const char *path);

int engine_fs_write_le_string(struct xfs_engine_mount_t *mnt, struct xfs_handle_t *fh, const char *s);

int engine_fs_read_entire(struct xfs_engine_mount_t *mnt, struct xfs_handle_t *h, uint8_t **out, size_t *out_len);
