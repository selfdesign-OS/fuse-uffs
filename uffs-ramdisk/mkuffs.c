/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall uffs.c `pkg-config fuse --cflags --libs` -o uffs
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <fnmatch.h>

#include "uffs_tree.h"
#include "uffs_types.h"

uffs_Device dev = {0};

int uffs_init()
{
	fprintf(stdout, "[uffs_init] called\n");

	uffs_TreeInit(&dev);
	uffs_BuildTree(&dev);
	
	fprintf(stdout, "[uffs_init] finished\n");
	return 0;
}

int uffs_getattr(const char *path, struct stat *stbuf)
{
	fprintf(stdout, "[uffs_getattr] called\n");
	fprintf(stdout, "[uffs_getattr] path: %s\n", path);

	TreeNode *node;
	URET result;
	
	memset(stbuf, 0, sizeof(struct stat));

	if (strcmp(path, "/") == 0) {
		result = uffs_TreeFindNodeByName(&dev, &node, path, DIR);
	}
	else {
		if (result = uffs_TreeFindNodeByName(&dev, &node, path, UDIR) != U_SUCC) // try file
			result = uffs_TreeFindNodeByName(&dev, &node, path, DIR); // try dir
	}
	if (result != U_SUCC) {
		fprintf(stderr, "[uffs_getattr] result is U_FAIL\n");
	}

	stbuf->st_mode = S_IFDIR | node->info.mode;
	stbuf->st_nlink = node->info.nlink;	
	stbuf->st_size = node->info.len;
	
	fprintf(stdout, "[uffs_getattr] finished\n");
	return 0;
}

int uffs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	fprintf(stdout, "[uffs_readdir] called\n");

	fprintf(stdout, "[uffs_readdir] finished\n");
	return 0;
}

struct fuse_operations uffs_oper = {
	.init		= uffs_init,
	.getattr	= uffs_getattr,
	.readdir	= uffs_readdir,
};

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <mount-directory>\n", argv[0]);
        return -1;
    }

    fprintf(stderr, "[main] init finished\n");

    return fuse_main(argc, argv, &uffs_oper, NULL);
}
