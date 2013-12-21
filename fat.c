/*
	clang -Wall fat.c `pkg-config fuse --cflags --libs` -o ./bin/fat
*/

#define FUSE_USE_VERSION 29

#include <errno.h>
#include <fuse.h>
#include <mcheck.h>
#include <stdio.h>
#include <stdlib.h>	
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


#define IMAGE_FILE_NAME "data"
#define NAME_LENGTH 16
#define CLUSTER_SIZE 512
#define CLUSTER_COUNT 100
#define END_OF_FILE 0xFFFFFFFF
#define EMPTY 0x00000000

typedef unsigned int cluster_t;

struct file_entry_s {
	char name[NAME_LENGTH];
	mode_t mode;
	time_t created;
	cluster_t cluster;
	size_t size;
};

typedef struct file_entry_s file_entry;

int image;
off_t data_offset;
int entries_count;

cluster_t get_next_cluster(cluster_t cluster) {
	lseek(image, sizeof(cluster_t) * cluster, SEEK_SET);
	cluster_t next;
	read(image, &next, sizeof(cluster_t));
	return next;
}

int is_cluster_free(cluster_t cluster) {
	return get_next_cluster(cluster) == 0 ? 1 : 0;
}

cluster_t find_empty_cluster_after(cluster_t cluster) {
	cluster_t current = cluster;
	while (!is_cluster_free(cluster) && cluster < CLUSTER_COUNT) {
		cluster++;
	}
	return cluster == CLUSTER_COUNT ? END_OF_FILE : cluster;
}

cluster_t find_empty_cluster() {
	return find_empty_cluster_after(0);
}

char* extract_folder(const char *path) {
	int length = strlen(path);
	int i = length - 1;
	while (i > 0 && path[i] != '/') {
		--i;
		
	}
	printf("Folder last '/' %d\n", i);
	printf("start malloc folder\n");
	char *result = (char *)malloc(length+1);
	printf("finish malloc folder\n");
	strcpy(result, path);
	result[i] = '\0';	
	printf("Folder: %s %d\n", result, strlen(result));
	return result;
}

char* extract_filename(const char *path) {
	int length = strlen(path);
	int i = length - 1;
		while (i > 0 && path[i] != '/') {
		--i;
	}
	printf("File last '/' %d\n", i);
	if (i < 0) {
		return NULL;
	}
	printf("start malloc name\n");
	char *result = (char *)malloc(length - i);
	printf("finish malloc name\n");
	strcpy(result, path + i + 1);
	printf("Name: %s %d\n", result, strlen(result));
	return result;
}

int get_file_entry_from_cluster(const char *name, cluster_t cluster, file_entry *file) {
	printf("cluster %d\n", cluster);
	lseek(image, data_offset + cluster * CLUSTER_SIZE, SEEK_SET);
	int i;
	file_entry tmp;
	for (i = 0; i<entries_count; i++) {
		read(image, &tmp, sizeof(file_entry));
		if (strcmp(name, (char*)tmp.name) == 0) {
			*file = tmp;
			return 0;	
		}
	}
	return 1;
}

int get_file_entry(const char *path, file_entry *file) {
	printf("Path: %s\n", path);
	if (strcmp(path, "") == 0) {
		file_entry root;
		root.cluster = 0;
		root.mode = S_IFDIR | 0755;
		*file = root;
		return 0;
	}
	file_entry tmp;
	if (get_file_entry(extract_folder(path), &tmp) != 0) {
		return 1;
	}
	if (get_file_entry_from_cluster(extract_filename(path), tmp.cluster, file) != 0) {
		return 1;
	}
	return 0;
}

int fat_getattr(const char *path, struct stat *stat) {
	memset(stat, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stat->st_mode = S_IFDIR | 0755;
		stat->st_nlink = 2;
	} else {
		file_entry file;
		if (get_file_entry(path, &file) != 0) {
			return -ENOENT;
		}
		stat->st_mode = S_IFREG | 0444;
		stat->st_nlink = 1;
		stat->st_size = file.size;
	}
	return 0;
}

int fat_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info) {
	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	file_entry folder;
	get_file_entry(path, &folder);
	cluster_t cluster = folder.cluster;

	int i;
	file_entry tmp;
	char *filename = malloc(NAME_LENGTH);

	do {
		lseek(image, data_offset, SEEK_SET);
		for (i = 0; i<entries_count; i++) {
			read(image, &tmp, sizeof(file_entry));
			if (strlen(tmp.name) != 0) {
				memcpy(filename, tmp.name, NAME_LENGTH);
				filler(buf, filename, NULL, 0);
			} 
		}
		cluster = get_next_cluster(cluster);
	} while (cluster != END_OF_FILE);
	return 0;
}
int fat_open (const char *path, struct fuse_file_info *info) {
	file_entry file;
	if (get_file_entry(path, &file) != 0) {
		return -ENOENT;
	}
	return 0;
}

int fat_read(const char *path, char *buf, size_t buf_size, off_t offset, struct fuse_file_info *info) {
	file_entry file;
	size_t size = 0;
	if (get_file_entry(path, &file) != 0) {
		return -ENOENT;
	}
	if (offset < file.size) {
		if (offset + buf_size > file.size) {
			size = file.size - offset;
		} else {
			size = buf_size;
		}
		lseek(image, data_offset + file.cluster * CLUSTER_SIZE, SEEK_SET);
		read(image, buf, size);
	}
	return size;
} 

static struct fuse_operations fat_oper = {
    .getattr    = fat_getattr,
    .readdir    = fat_readdir,
    .read       = fat_read,
    .open 		= fat_open
};

int main(int argc, char *argv[]) {
	image = open(IMAGE_FILE_NAME, O_RDWR);
	if (image == -1) {
		printf("Error!\n");
		return 1;
	}
	data_offset = sizeof(cluster_t) * CLUSTER_COUNT;
	entries_count = CLUSTER_SIZE / sizeof(file_entry);
	return fuse_main(argc, argv, &fat_oper, NULL);
}