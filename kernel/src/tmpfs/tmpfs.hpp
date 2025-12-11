#ifndef TMPFS_HPP
#define TMPFS_HPP 1

#include <cstddef>
#include <cstdint>
#include <types.hpp>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR 0x0002
#define O_CREAT 0x0040 
#define O_EXCL 0x0080
#define O_TRUNC 0x0200
#define O_APPEND 0x0400
#define O_DIRECTORY 0x10000
#define O_CLOEXEC 0x80000

#define O_BUILTIN_DEVICE_FILE 0x80000000

#define S_IRUSR 0x0100
#define S_IWUSR 0x0080
#define S_IXUSR 0x0040
#define S_IRGRP 0x0020
#define S_IWGRP 0x0010
#define S_IXGRP 0x0008
#define S_IROTH 0x0004
#define S_IWOTH 0x0002
#define S_IXOTH 0x0001

#define S_IFDIR 0040000
#define S_IFREG 0100000

#define TIME_SIZE 24

struct stat {
    uint64_t st_dev;
    uint64_t st_ino;
    mode_t st_mode;
    uint32_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    uint64_t rdev;
    off_t st_size;
    uint64_t st_blksize;
    uint64_t st_blocks;
    uint8_t st_atime[TIME_SIZE];
    uint8_t st_mtime[TIME_SIZE];
    uint8_t st_ctime[TIME_SIZE];
    uint32_t st_result_mask;
    uint32_t st_attributes;
    uint64_t st_change_cookie;
};

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
    bool isdev;
    char devpath[256];
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

namespace tmpfs {

void initialise();

int chdir(const char* path);
int mkdir(const char* path, mode_t mode);
int mkdirat(int dirfd, const char* path, mode_t mode);

int open(const char* path, int flags, mode_t mode = 0755, char* devpath = nullptr);
int openat(int dirfd, const char* path, int flags, mode_t mode = 0755);
int close(int fd);
ssize_t read(int fd, void* buf, size_t count);
ssize_t write(int fd, const void* buf, size_t count);
ssize_t pread(int fd, void* buf, size_t count, off_t offset);
ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset);
off_t lseek(int fd, off_t offset, int whence);
int fstat(int fd, struct stat* buf);
int fchmod(int fd, mode_t mode);
int fchown(int fd, uid_t owner, gid_t group);
int ftruncate(int fd, off_t length);
int fsync(int fd);
int fdatasync(int fd);

int link(const char* oldpath, const char* newpath);
int unlink(const char* path);
int rename(const char* oldpath, const char* newpath);
int symlink(const char* target, const char* linkpath);
ssize_t readlink(const char* path, char* buf, size_t bufsize);

int rmdir(const char* path);
ssize_t getdents(int fd, void* buf, size_t bufsize);

void load_initrd(void* base, size_t size);
void list_initrd();

void build_tree(node_struct* node, char* prefix);
void print_tree();

}

#endif
