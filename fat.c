#define FUSE_USE_VERSION 29

#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define IMAGE_FILE_NAME "data"
#define NAME_LENGTH 19
#define CLUSTER_SIZE 512
#define CLUSTER_COUNT 100

typedef unsigned int cluster_t;

struct fat_entry {
	char *name[NAME_LENGTH];
	char permissions;
	time_t created;
	cluster_t cluster;
	size_t size;
};

int data_offset;

int fat_getattr(const char *path, struct stat *stat) {
	int res = 0;

	memset(stat, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stat->st_mode = S_IFDIR | 0755;
		stat->st_nlink = 2;
	} else
		res = -ENOENT;

	return res;
}


int fat_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info) {
	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	return 0;
}

int fat_open(const char *path, struct fuse_file_info *info) {
	return 0;
}

int fat_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *info) {
	return 0;
}

int fat_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *info) {
	return 0;
}

int fat_create(const char *path, mode_t mode, struct fuse_file_info *info) {
	return 0;
}

int fat_truncate(const char *path, off_t offset) {
	return 0;
}

int fat_release(const char *path, struct fuse_file_info *info) {
	return 0;
}

int fat_flush(const char *path, struct fuse_file_info *info) {
	return 0;
}

int fat_unlink(const char *path) {
	return 0;
}



int fat_mkdir(const char *path, mode_t mode) {
	return 0;
}

int fat_rmdir(const char *path) {
	return 0;
}		


static struct fuse_operations fat_oper = {
    .getattr    = fat_getattr,
    .readdir    = fat_readdir,
    .open       = fat_open,
    .read       = fat_read
};

int main(int argc, char *argv[]) {
	return fuse_main(argc, argv, &fat_oper, NULL);
}