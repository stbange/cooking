#ifndef FD_VFS_H
#define FD_VFS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FD_VFS_MAX_ENTRIES 256
#define FD_VFS_MAX_PATH    128

typedef struct {
    char     path[FD_VFS_MAX_PATH];
    uint8_t *data;
    size_t   size;
    bool     is_override;
} fd_vfs_entry_t;

typedef struct {
    char           base_dir[512];
    int            count;
    fd_vfs_entry_t entries[FD_VFS_MAX_ENTRIES];
} fd_vfs_t;

/* Initialize VFS pointing at sibling ./assets/ directory (or explicit root) */
void fd_vfs_init(fd_vfs_t *vfs, const char *explicit_assets_dir, const char *exe_base_path);
void fd_vfs_free(fd_vfs_t *vfs);

/* Validate relative path (rejects empty, leading '/', '\\', or '..') */
bool fd_vfs_valid_relpath(const char *rel_path);

/* Parse flat JSON string array ["path/a", "path/b"] from manifest.json */
int fd_vfs_parse_manifest(const char *json, size_t json_len,
                          char out_paths[][FD_VFS_MAX_PATH], int max_paths);

/* Store pre-fetched file bytes (used by Web HTTP fetch) or runtime script override */
bool fd_vfs_put(fd_vfs_t *vfs, const char *rel_path, const void *data, size_t size, bool is_override);
bool fd_vfs_clear_override(fd_vfs_t *vfs, const char *rel_path);

/* Read external asset into newly malloc'd null-terminated buffer (caller frees out_buf) */
bool fd_vfs_read(const fd_vfs_t *vfs, const char *rel_path, uint8_t **out_buf, size_t *out_size);

#endif /* FD_VFS_H */
