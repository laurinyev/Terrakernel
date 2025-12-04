#include "tmpfs.hpp"
#include <cstring>
#include <mem/mem.hpp>
#include <drivers/serial/print.hpp>

namespace tmpfs {

struct node_struct {
    char name[128];
    bool is_dir;
    bool is_symlink;
    node_struct* first_child;
    node_struct* next_sibling;
    node_struct* parent;
    char* content;
    size_t size;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    int refcount;
};

struct filedesc {
    int fd;
    node_struct* node;
    off_t offset;
    int flags;
    mode_t mode;
    bool is_dirent;
    bool free;
};

static node_struct* root = nullptr;
static node_struct* cwd = nullptr;

static struct {
    filedesc fd[256];
} ftable;

char* build_path_at_node(node_struct* n) {
    if (!n) return nullptr;

    const size_t MAX_PATH = 1024;
    char tmp[MAX_PATH] = {0};
    char* ptr = tmp + MAX_PATH - 1;
    *ptr = '\0';

    node_struct* curr = n;
    while (curr) {
        if (curr->parent == nullptr) {
            if (ptr == tmp + MAX_PATH - 1) {
                *(--ptr) = '/';
            }
            break;
        } else {
            size_t len = strlen(curr->name);
            if (ptr - tmp < (len + 1)) {
                return nullptr;
            }
            ptr -= len;
            mem::memcpy(ptr, curr->name, len);
            *(--ptr) = '/';
        }
        curr = curr->parent;
    }

    size_t total_len = tmp + MAX_PATH - ptr;
    char* result = (char*)mem::heap::malloc(total_len);
    if (!result) return nullptr;
    mem::memcpy(result, ptr, total_len);
    return result;
}

static int alloc_fd() {
    for (int i = 0; i < 256; i++) {
        if (ftable.fd[i].free) {
            ftable.fd[i].free = false;
            ftable.fd[i].offset = 0;
            ftable.fd[i].node = nullptr;
            ftable.fd[i].flags = 0;
            ftable.fd[i].mode = 0;
            return ftable.fd[i].fd = i;
        }
    }
    return -1;
}

static void free_node(node_struct* n) {
    if (!n) return;
    if (--n->refcount > 0) return;
    if (n->content) mem::heap::free(n->content);
    mem::heap::free(n->name);
    mem::heap::free(n);
}

static node_struct* create_at_path_internal(node_struct* base, const char* path, bool is_dir, mode_t mode) {
    if (!base || !path) return nullptr;
    
    char tmp[512];
    strncpy(tmp, path, 511);
    tmp[511] = '\0';
    
    node_struct* curr = (path[0] == '/') ? root : base;
    node_struct* prev = nullptr;
    char* token = strtok(tmp, "/");
    
    while (token) {
        node_struct* child = curr->first_child;
        prev = nullptr;
        while (child && strcmp(child->name, token) != 0) {
            prev = child;
            child = child->next_sibling;
        }
        
        char* next_token = strtok(nullptr, "/");
        bool last = (next_token == nullptr);
        
        if (!child) {
            child = (node_struct*)mem::heap::malloc(sizeof(node_struct));
            if (!child) return nullptr;
            mem::memset(child, 0, sizeof(node_struct));

            strncpy(child->name, token, sizeof(child->name) - 1);
            child->name[sizeof(child->name) - 1] = '\0';
            
            child->parent = curr;
            child->first_child = nullptr;
            child->next_sibling = nullptr;
            child->content = nullptr;
            child->size = 0;
            child->mode = last ? mode : 0755;
            child->uid = child->gid = 0;
            child->refcount = 1;
            child->is_dir = last ? is_dir : true;
            child->is_symlink = false;
            
            if (!curr->first_child) curr->first_child = child;
            else prev->next_sibling = child;
        }
        
        curr = child;
        token = next_token;
    }

    return curr;
}

static node_struct* create_at_path(char* path, bool is_dir, mode_t mode) {
	return create_at_path_internal(cwd, path, is_dir, mode);
}

static node_struct* resolve_path_internal(node_struct* base, const char* path) {
    if (!base || !path) {
        return nullptr;
    }
    
    char tmp[512];
    strncpy(tmp, path, 511);
    tmp[511] = '\0';
    
    node_struct* curr = (path[0] == '/') ? root : base;

    char* token = strtok(tmp, "/");
    
    while (token) {
        if (strcmp(token, ".") == 0) {
        } else if (strcmp(token, "..") == 0) {
            if (curr->parent) {
                curr = curr->parent;
            } else {
            }
        } else {
            node_struct* child = curr->first_child;
            while (child && strcmp(child->name, token) != 0) {
                child = child->next_sibling;
            }
            if (!child) {
                return nullptr;
            }
            curr = child;
        }

        token = strtok(nullptr, "/");
    }
    
    return curr;
}

node_struct* resolve_path(const char* path) {
    return resolve_path_internal(cwd, path);
}

void initialise() {
    for (int i = 0; i < 256; i++) {
        ftable.fd[i].fd = i;
        ftable.fd[i].free = true;
        ftable.fd[i].node = nullptr;
    }
    root = (node_struct*)mem::heap::malloc(sizeof(node_struct));

    strncpy(root->name, "/", sizeof(root->name) - 1);
    root->name[sizeof(root->name) - 1] = '\0';
    
    root->is_dir = true;
    root->is_symlink = false;
    root->first_child = nullptr;
    root->next_sibling = nullptr;
    root->parent = nullptr;
    root->content = nullptr;
    root->size = 0;
    root->mode = 0755;
    root->uid = root->gid = 0;
    root->refcount = 1;
    cwd = root;
}

int chdir(const char* path) {
    node_struct* n = resolve_path(path);
    if (!n) return -1;
    cwd = n;
    return 0;
}

int mkdir(const char* path, mode_t mode) {
    bool created;
    node_struct* n = create_at_path((char*)path, true, mode);
    if (!n) return -1;
    n->is_dir = true;
    n->first_child = nullptr;
    n->next_sibling = nullptr;
    n->content = nullptr;
    n->size = 0;
    n->mode = mode;
    return 0;
}

int mkdirat(int dirfd, const char* path, mode_t mode) {
    node_struct* saved = cwd;
    if (dirfd >= 0 && dirfd < 256 && ftable.fd[dirfd].node) cwd = ftable.fd[dirfd].node;
    int ret = mkdir(path, mode);
    cwd = saved;
    return ret;
}

int rmdir(const char* path) {
    node_struct* n = resolve_path(path);
    if (!n || !n->is_dir || n->first_child) return -1;
    node_struct* parent = n->parent;
    if (!parent) return -1;
    if (parent->first_child == n) parent->first_child = n->next_sibling;
    else {
        node_struct* s = parent->first_child;
        while (s && s->next_sibling != n) s = s->next_sibling;
        if (s) s->next_sibling = n->next_sibling;
    }
    free_node(n);
    return 0;
}

int open(const char* path, int flags, mode_t mode) {
    if (!path) return -1;

    node_struct* n = nullptr;
    bool created = false;

    if (flags & O_CREAT) {
        n = resolve_path(path);
        if (!n) {
            n = create_at_path((char*)path, false, mode);
            if (!n) return -2;
            created = true;
        }
    } else {
        n = resolve_path(path);
        if (!n) return -3;
    }

    // kaboom you if (n->is_dir && !(flags & O_DIRECTORY)) return -1;

    int fd = alloc_fd();
    if (fd < 0) return -4;

    ftable.fd[fd].node = n;
    ftable.fd[fd].flags = flags;
    ftable.fd[fd].mode = mode;
    ftable.fd[fd].offset = 0;
    ftable.fd[fd].is_dirent = n->is_dir;
    n->refcount++;

    return fd;
}

int openat(int dirfd, const char* path, int flags, mode_t mode) {
    node_struct* saved = cwd;
    if (dirfd >= 0 && dirfd < 256 && ftable.fd[dirfd].node) cwd = ftable.fd[dirfd].node;
    int fd = open(path, flags, mode);
    cwd = saved;
    return fd;
}

int close(int fd) {
    if (fd < 0 || fd >= 256) return -1;
    if (ftable.fd[fd].node) free_node(ftable.fd[fd].node);
    ftable.fd[fd].node = nullptr;
    ftable.fd[fd].free = true;
    return 0;
}

ssize_t read(int fd, void* buf, size_t count) {
    if (fd < 0 || fd >= 256) return -1;
    filedesc* f = &ftable.fd[fd];
    if (!f->node || f->node->is_dir || f->node->is_symlink) return -1;
    size_t rem = f->node->size - f->offset;
    size_t to_read = count < rem ? count : rem;
    if (to_read > 0) mem::memcpy(buf, f->node->content + f->offset, to_read);
    f->offset += to_read;
    return to_read;
}

ssize_t pread(int fd, void* buf, size_t count, off_t offset) {
    off_t orig = ftable.fd[fd].offset;
    lseek(fd, offset, SEEK_SET);
    ssize_t ret = read(fd, buf, count);
    ftable.fd[fd].offset = orig;
    return ret;
}

ssize_t write(int fd, const void* buf, size_t count) {
    if (fd < 0 || fd >= 256) return -1;
    filedesc* f = &ftable.fd[fd];
    if (!f->node || f->node->is_dir || f->node->is_symlink) return -1;

    size_t new_size = f->offset + count;
    if (!f->node->content) f->node->content = (char*)mem::heap::malloc(new_size);
    else {
        char* tmp = (char*)mem::heap::realloc(f->node->content, new_size);
        if (!tmp) return -1;
        f->node->content = tmp;
    }

    mem::memcpy(f->node->content + f->offset, buf, count);
    f->offset += count;
    if (f->offset > f->node->size) f->node->size = f->offset;
    return count;
}

ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset) {
    off_t orig = ftable.fd[fd].offset;
    lseek(fd, offset, SEEK_SET);
    ssize_t ret = write(fd, buf, count);
    ftable.fd[fd].offset = orig;
    return ret;
}

off_t lseek(int fd, off_t offset, int whence) {
    if (fd < 0 || fd >= 256) return -1;
    filedesc* f = &ftable.fd[fd];
    if (!f->node) return -1;
    off_t new_offset = f->offset;
    switch (whence) {
        case SEEK_SET: new_offset = offset; break;
        case SEEK_CUR: new_offset += offset; break;
        case SEEK_END: new_offset = f->node->size + offset; break;
        default: return -1;
    }
    if (new_offset < 0) return -1;
    f->offset = new_offset;
    return new_offset;
}

int fstat(int fd, struct stat* buf) {
    if (fd < 0 || fd >= 256) return -1;
    filedesc* f = &ftable.fd[fd];
    if (!f->node) return -1;
    buf->st_mode = f->node->is_dir ? (S_IFDIR | f->node->mode) : (S_IFREG | f->node->mode);
    buf->st_size = f->node->size;
    buf->st_uid = f->node->uid;
    buf->st_gid = f->node->gid;
    return 0;
}

int fchmod(int fd, mode_t mode) {
    if (fd < 0 || fd >= 256) return -1;
    if (!ftable.fd[fd].node) return -1;
    ftable.fd[fd].node->mode = mode;
    return 0;
}

int fchown(int fd, uid_t owner, gid_t group) {
    if (fd < 0 || fd >= 256) return -1;
    if (!ftable.fd[fd].node) return -1;
    ftable.fd[fd].node->uid = owner;
    ftable.fd[fd].node->gid = group;
    return 0;
}

int ftruncate(int fd, off_t length) {
    if (fd < 0 || fd >= 256) return -1;
    filedesc* f = &ftable.fd[fd];
    if (!f->node || f->node->is_dir || f->node->is_symlink) return -1;

    if (length < f->node->size) {
        char* tmp = (char*)mem::heap::realloc(f->node->content, length);
        if (!tmp && length > 0) return -1;
        f->node->content = tmp;
        f->node->size = length;
        if (f->offset > length) f->offset = length;
    } else if (length > f->node->size) {
        char* tmp = (char*)mem::heap::realloc(f->node->content, length);
        if (!tmp) return -1;
        mem::memset(tmp + f->node->size, 0, length - f->node->size);
        f->node->content = tmp;
        f->node->size = length;
    }
    return 0;
}

int fsync(int fd) { return 0; }
int fdatasync(int fd) { return 0; }

int link(const char* oldpath, const char* newpath) {
    node_struct* n = resolve_path(oldpath);
    if (!n || n->is_dir) return -1;

    bool created;
    node_struct* newnode = create_at_path((char*)newpath, false, 0755);
    if (!newnode || !created) return -1;

    newnode->is_dir = n->is_dir;
    newnode->is_symlink = n->is_symlink;
    newnode->content = n->content;
    newnode->size = n->size;
    newnode->mode = n->mode;
    newnode->uid = n->uid;
    newnode->gid = n->gid;
    newnode->refcount = n->refcount;
    n->refcount++;
    return 0;
}

int unlink(const char* path) {
    node_struct* n = resolve_path(path);
    if (!n || n->is_dir) return -1;
    node_struct* parent = n->parent;
    if (!parent) return -1;
    if (parent->first_child == n) parent->first_child = n->next_sibling;
    else {
        node_struct* sibling = parent->first_child;
        while (sibling && sibling->next_sibling != n) sibling = sibling->next_sibling;
        if (sibling) sibling->next_sibling = n->next_sibling;
    }
    free_node(n);
    return 0;
}

int rename(const char* oldpath, const char* newpath) {
    node_struct* n = resolve_path(oldpath);
    if (!n) return -1;
    unlink(newpath);
    char* name = strrchr(newpath, '/');
    name = name ? name + 1 : (char*)newpath;
    mem::heap::free(n->name);

    strncpy(n->name, name, sizeof(n->name) - 1);
    n->name[sizeof(n->name) - 1] = '\0';

    return 0;
}

int symlink(const char* target, const char* linkpath) {
    node_struct* target_node = resolve_path(target);
    if (!target_node) return -1;

    char* abs_target = build_path_at_node(target_node);
    if (!abs_target) return -1;

    node_struct* n = (node_struct*)mem::heap::malloc(sizeof(node_struct));
    mem::memset(n, 0, sizeof(node_struct));

    char* name = strrchr(linkpath, '/');
    name = name ? name + 1 : (char*)linkpath;

    strncpy(n->name, name, sizeof(n->name) - 1);
    n->name[sizeof(n->name) - 1] = '\0';

    n->is_dir = false;
    n->is_symlink = true;
    n->content = abs_target;
    n->size = strlen(abs_target);
    n->first_child = nullptr;
    n->next_sibling = nullptr;
    n->parent = cwd;
    n->mode = 0777;
    n->uid = n->gid = 0;
    n->refcount = 1;

    if (!cwd->first_child) cwd->first_child = n;
    else {
        node_struct* s = cwd->first_child;
        while (s->next_sibling) s = s->next_sibling;
        s->next_sibling = n;
    }

    return 0;
}

ssize_t readlink(const char* path, char* buf, size_t bufsize) {
    node_struct* n = resolve_path(path);
    if (!n || !n->is_symlink) return -1;
    strncpy(buf, n->content, bufsize);
    return strlen(buf);
}

ssize_t getdents(int fd, void* buf, size_t bufsize) {
    if (fd < 0 || fd >= 256) return -1;
    filedesc* f = &ftable.fd[fd];
    if (!f->node || !f->node->is_dir) return -1;
    char* out = (char*)buf;
    size_t used = 0;
    node_struct* c = f->node->first_child;
    while (c) {
        size_t len = strlen(c->name);
        if (used + len + 1 > bufsize) break;
        strcpy(out + used, c->name);
        used += len + 1;
        c = c->next_sibling;
    }
    return used;
}

}
