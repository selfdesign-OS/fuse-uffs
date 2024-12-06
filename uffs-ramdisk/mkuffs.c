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

#include "uffs_device.h"
#include "uffs_tree.h"
#include "uffs_types.h"

uffs_Device dev = {0};

int uffs_init()
{
	fprintf(stdout, "[uffs_init] called");

	uffs_TreeInit(&dev);
	uffs_BuildTree(&dev);
	
	return 0;
}

int uffs_getattr(const char *path, struct stat *stbuf)
{
	fprintf(stdout, "[uffs_getattr] called");
	fprintf(stdout, "[uffs_getattr] path: %s", path);

	TreeNode *node;
	URET result;
	uffs_Device
	
	memset(stbuf, 0, sizeof(struct stat));

	if (strcmp(path, "/") == 0) {
		result = uffs_TreeFindNodeByName(dev, node, path, DIR);
	}
	else {
		if (result = uffs_TreeFindNodeByName(dev, node, path, UDIR) != U_SUCC) // try file
			result = uffs_TreeFindNodeByName(dev, node, path, DIR); // try dir
	}

	stbuf->st_mode = S_IFDIR | node->info.mode;
	stbuf->st_nlink = node->info.nlink;	
	stbuf->st_size = node->info.len;
	
	return 0;
}

int uffs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;
	int i,index = -1;
	char *dirpath = "";
	
	if (strcmp(path, "/") != 0 ) {
		for (i = 0; i < f.nnodes; i++) {
			if ( f.nodes[i].is_dir == 1 && f.nodes[i].status == used && strcmp(path, f.nodes[i].path) == 0) {
				index = i;
				dirpath = f.nodes[i].path;
				break;
			}
		}

		if (index == -1)
			return -ENOENT;
	
	}
	
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	for (i = 0; i < f.nnodes; i++) {
		if ( f.nodes[i].status == used && pathmatch(dirpath, f.nodes[i].path) == 0 ) {
			filler(buf, strrchr(f.nodes[i].path, '/')+1 , NULL, 0);
		}
	}

	return 0;
}

struct fuse_operations uffs_oper = {
	.init		= uffs_init,
	.getattr	= uffs_getattr,
	.readdir	= uffs_readdir,
};

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <mount-directory> <sizeinMB> [<disk-image>]\n", argv[0]);
		return -1;
	}

	if (argc == 4) {
		strcpy(filename, argv[3]);
		persistent = 1;
	}

	size_t size_bytes = atoi(argv[2])*1000000;
	NBLOCKS = size_bytes/(sizeof(node) + sizeof(block));
	NNODES = NBLOCKS;
	
	//size_t storage = NBLOCKS*sizeof(block);
	//fprintf(stderr,"number of blocks: %d\n", NBLOCKS);
	//fprintf(stderr,"number of nodes: %d\n", NNODES);
	//fprintf(stderr,"Total space for storage: %lu\n", storage);

	uffs_init();

	argc = 2;
	return fuse_main(argc, argv, &uffs_oper, NULL);
}
