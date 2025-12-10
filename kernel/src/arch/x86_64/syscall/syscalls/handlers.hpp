#ifndef HANDLERS_HPP
#define HANDLERS_HPP 1

uint64_t sys_open(const char* path, int flags, mode_t mode);
uint64_t sys_close(int fd);
uint64_t sys_read(int fd, void* buf, size_t count);
uint64_t sys_write(int fd, const void* buf, size_t count);
uint64_t sys_pread(int fd, void* buf, size_t count, off_t offset);
uint64_t sys_pwrite(int fd, const void* buf, size_t count, off_t offset);
uint64_t sys_lseek(int fd, off_t offset, int whence);
uint64_t sys_stat(int fd, void* buf);
uint64_t sys_chmod(int fd, mode_t mode);
uint64_t sys_chown(int fd, uid_t owner, gid_t group);
uint64_t sys_truncate(int fd, off_t length);
uint64_t sys_sync(int fd);
uint64_t sys_datasync(int fd);
uint64_t sys_mkdir(const char* path, mode_t mode);
uint64_t sys_chdir(const char* path);
uint64_t sys_link(const char* oldpath, const char* newpath);
uint64_t sys_unlink(const char* path);
uint64_t sys_rename(const char* oldpath, const char* newpath);
uint64_t sys_symlink(const char* target, const char* linkpath);
uint64_t sys_readlink(const char* path, char* buf, size_t bufsize);
uint64_t sys_rmdir(const char* path);
uint64_t sys_getdents(int fd, void* buf, size_t bufsize);

    #endif