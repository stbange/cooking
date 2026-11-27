#include "fd_vfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool probe_manifest(const char *dir) {
    char buf[600];
    snprintf(buf, sizeof(buf), "%s/manifest.json", dir);
    FILE *f = fopen(buf, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

void fd_vfs_init(fd_vfs_t *vfs, const char *explicit_assets_dir, const char *exe_base_path) {
    memset(vfs, 0, sizeof(*vfs));
    const char *env_dir = getenv("FATDUCK_ASSETS_DIR");
    if (explicit_assets_dir && explicit_assets_dir[0] != '\0') {
        snprintf(vfs->base_dir, sizeof(vfs->base_dir), "%s", explicit_assets_dir);
        return;
    }
    if (env_dir && env_dir[0] != '\0' && probe_manifest(env_dir)) {
        snprintf(vfs->base_dir, sizeof(vfs->base_dir), "%s", env_dir);
        return;
    }
    if (exe_base_path && exe_base_path[0] != '\0') {
        char candidate[512];
        snprintf(candidate, sizeof(candidate), "%sassets", exe_base_path);
        if (probe_manifest(candidate)) {
            snprintf(vfs->base_dir, sizeof(vfs->base_dir), "%s", candidate);
            return;
        }
        snprintf(candidate, sizeof(candidate), "%s../assets", exe_base_path);
        if (probe_manifest(candidate)) {
            snprintf(vfs->base_dir, sizeof(vfs->base_dir), "%s", candidate);
            return;
        }
    }
    if (probe_manifest("assets")) {
        snprintf(vfs->base_dir, sizeof(vfs->base_dir), "assets");
    } else if (probe_manifest("../assets")) {
        snprintf(vfs->base_dir, sizeof(vfs->base_dir), "../assets");
    } else {
        snprintf(vfs->base_dir, sizeof(vfs->base_dir), "assets");
    }
}

void fd_vfs_free(fd_vfs_t *vfs) {
    if (!vfs) return;
    for (int i = 0; i < vfs->count; i++) {
        free(vfs->entries[i].data);
        vfs->entries[i].data = NULL;
    }
    vfs->count = 0;
}

bool fd_vfs_valid_relpath(const char *p) {
    if (!p || p[0] == '\0' || p[0] == '/' || p[0] == '\\') return false;
    size_t len = strlen(p);
    if (len >= FD_VFS_MAX_PATH) return false;
    for (size_t i = 0; i < len; i++) {
        if (p[i] == '\\') return false;
        if (p[i] == '.' && p[i + 1] == '.') return false;
    }
    return true;
}

int fd_vfs_parse_manifest(const char *json, size_t json_len,
                          char out_paths[][FD_VFS_MAX_PATH], int max_paths) {
    if (!json || max_paths <= 0) return 0;
    int found = 0;
    size_t i = 0;
    while (i < json_len && found < max_paths) {
        if (json[i] == '"') {
            i++;
            char tmp[FD_VFS_MAX_PATH];
            size_t k = 0;
            bool ok = true;
            while (i < json_len && json[i] != '"') {
                if (json[i] == '\\' && i + 1 < json_len) i++;
                if (k + 1 < sizeof(tmp)) {
                    tmp[k++] = json[i];
                } else {
                    ok = false;
                }
                i++;
            }
            tmp[k] = '\0';
            if (i < json_len && json[i] == '"') i++;
            if (ok && fd_vfs_valid_relpath(tmp)) {
                memcpy(out_paths[found], tmp, k + 1);
                found++;
            }
        } else {
            i++;
        }
    }
    return found;
}

static int find_entry(const fd_vfs_t *vfs, const char *rel_path) {
    for (int i = 0; i < vfs->count; i++) {
        if (strcmp(vfs->entries[i].path, rel_path) == 0) return i;
    }
    return -1;
}

bool fd_vfs_put(fd_vfs_t *vfs, const char *rel_path, const void *data, size_t size, bool is_override) {
    if (!vfs || !fd_vfs_valid_relpath(rel_path) || (!data && size > 0)) return false;
    int idx = find_entry(vfs, rel_path);
    if (idx < 0) {
        if (vfs->count >= FD_VFS_MAX_ENTRIES) return false;
        idx = vfs->count++;
        snprintf(vfs->entries[idx].path, FD_VFS_MAX_PATH, "%s", rel_path);
    } else if (vfs->entries[idx].is_override && !is_override) {
        /* Preserve active runtime override when base file is refreshed */
        return true;
    }
    uint8_t *copy = (uint8_t *)malloc(size + 1);
    if (!copy) return false;
    if (size > 0) memcpy(copy, data, size);
    copy[size] = '\0';
    free(vfs->entries[idx].data);
    vfs->entries[idx].data = copy;
    vfs->entries[idx].size = size;
    vfs->entries[idx].is_override = is_override;
    return true;
}

bool fd_vfs_clear_override(fd_vfs_t *vfs, const char *rel_path) {
    if (!vfs || !rel_path) return false;
    int idx = find_entry(vfs, rel_path);
    if (idx < 0 || !vfs->entries[idx].is_override) return false;
    free(vfs->entries[idx].data);
    vfs->entries[idx] = vfs->entries[vfs->count - 1];
    memset(&vfs->entries[vfs->count - 1], 0, sizeof(fd_vfs_entry_t));
    vfs->count--;
    return true;
}

bool fd_vfs_read(const fd_vfs_t *vfs, const char *rel_path, uint8_t **out_buf, size_t *out_size) {
    if (!vfs || !out_buf || !fd_vfs_valid_relpath(rel_path)) return false;
    *out_buf = NULL;
    if (out_size) *out_size = 0;

    int idx = find_entry(vfs, rel_path);
    if (idx >= 0 && vfs->entries[idx].data) {
        size_t sz = vfs->entries[idx].size;
        uint8_t *copy = (uint8_t *)malloc(sz + 1);
        if (!copy) return false;
        if (sz > 0) memcpy(copy, vfs->entries[idx].data, sz);
        copy[sz] = '\0';
        *out_buf = copy;
        if (out_size) *out_size = sz;
        return true;
    }

    char full_path[680];
    snprintf(full_path, sizeof(full_path), "%s/%s", vfs->base_dir, rel_path);
    FILE *f = fopen(full_path, "rb");
    if (!f) return false;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return false;
    }
    long len = ftell(f);
    if (len < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return false;
    }
    uint8_t *buf = (uint8_t *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        return false;
    }
    size_t rd = (len > 0) ? fread(buf, 1, (size_t)len, f) : 0;
    fclose(f);
    if (rd != (size_t)len) {
        free(buf);
        return false;
    }
    buf[rd] = '\0';
    *out_buf = buf;
    if (out_size) *out_size = rd;
    return true;
}
