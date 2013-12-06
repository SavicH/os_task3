#define FUSE_USE_VERSION 29

#include <fuse.h>

static struct fuse_operations fat_oper;

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &fat_oper, NULL);
}