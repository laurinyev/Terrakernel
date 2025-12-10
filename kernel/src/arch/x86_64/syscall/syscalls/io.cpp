#include "../syscall.hpp"
#include <tmpfs/tmpfs.hpp>

uint64_t sys_open(const char* path, int flags, mode_t mode) {
    return tmpfs::open(path, flags, mode);
}

uint64_t sys_close(int fd) {
    return tmpfs::close(fd);
}

uint64_t sys_read(int fd, void* buf, size_t count) {
    return tmpfs::read(fd, buf, count);
}

uint64_t sys_write(int fd, const void* buf, size_t count) {
    return tmpfs::write(fd, buf, count);
}

uint64_t sys_pread(int fd, void* buf, size_t count, off_t offset) {
    return tmpfs::pread(fd, buf, count, offset);
}

uint64_t sys_pwrite(int fd, const void* buf, size_t count, off_t offset) {
    return tmpfs::pwrite(fd, buf, count, offset);
}

uint64_t sys_lseek(int fd, off_t offset, int whence) {
    return tmpfs::lseek(fd, offset, whence);
}

uint64_t sys_stat(int fd, void* buf) {
    return tmpfs::fstat(fd, (stat*)buf);
}

uint64_t sys_chmod(int fd, mode_t mode) {
    return tmpfs::fchmod(fd, mode);
}

uint64_t sys_chown(int fd, uid_t owner, gid_t group) {
    return tmpfs::fchown(fd, owner, group);
}

uint64_t sys_truncate(int fd, off_t length) {
    return tmpfs::ftruncate(fd, length);
}

uint64_t sys_sync(int fd) {
    return tmpfs::fsync(fd);
}

uint64_t sys_datasync(int fd) {
    return tmpfs::fdatasync(fd);
}

uint64_t sys_mkdir(const char* path, mode_t mode) {
    return tmpfs::mkdir(path, mode);
}

uint64_t sys_chdir(const char* path) {
    return tmpfs::chdir(path);
}

uint64_t sys_link(const char* oldpath, const char* newpath) {
    return tmpfs::link(oldpath, newpath);
}

uint64_t sys_unlink(const char* path) {
    return tmpfs::unlink(path);
}

uint64_t sys_rename(const char* oldpath, const char* newpath) {
    return tmpfs::rename(oldpath, newpath);
}

uint64_t sys_symlink(const char* target, const char* linkpath) {
    return tmpfs::symlink(target, linkpath);
}

uint64_t sys_readlink(const char* path, char* buf, size_t bufsize) {
    return tmpfs::readlink(path, buf, bufsize);
}

uint64_t sys_rmdir(const char* path) {
    return tmpfs::rmdir(path);
}

uint64_t sys_getdents(int fd, void* buf, size_t bufsize) {
    return tmpfs::getdents(fd, buf, bufsize);
}
