#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <gtest/gtest.h>

#include "tc_api.h"

#define POSIX_WARN(fmt, args...) fprintf(stderr, "==posix-WARN==" fmt, ##args)

/**
 * Ensure the file does not exist
 * before test.
 */
static void RemoveFile(const char *path)
{
	int r = unlink(path);
	EXPECT_TRUE(r == 0 || errno == ENOENT);
}

/**
 * Ensure files does not exist
 * before test.
 */
static void RemoveFiles(const char **path, int count)
{
	int i = 0, r = 0;

	while (i < count) {
		r = unlink(path[i]);
		EXPECT_TRUE(r == 0 || errno == ENOENT);
		i++;
	}
}

/**
 * Ensure Directory does not exist
 * before test.
 */
static void RemoveDir(const char **path, int count)
{
	int i = 0, r = 0;

	while (i < count) {
		r = rmdir(path[i]);
		EXPECT_TRUE(r == 0 || r == ENOENT);
		i++;
	}
}

/**
 * Free the tc_iovec
 */

void clear_iovec(tc_iovec *user_arg, int count)
{
	int i = 0;

	while (i < count) {
		free((user_arg + i)->data);
		i++;
	}

	free(user_arg);
}

/**
 * Set the tc_iovec
 */
static tc_iovec *set_iovec_file_paths(const char **PATH, int count,
				      int is_write)
{
	int i = 0;
	tc_iovec *user_arg = NULL;
	const int N = 4096;

	user_arg = (tc_iovec *)calloc(count, sizeof(tc_iovec));

	while (i < count) {
		if (PATH[i] == NULL) {
			POSIX_WARN(
			    "set_iovec_FilePath() failed for file : %s\n",
			    PATH[i]);

			int indx = 0;
			while (indx < i) {
				free((user_arg + indx)->data);
				indx++;
			}
			free(user_arg);

			return NULL;
		}

		(user_arg + i)->file.type = TC_FILE_PATH;
		(user_arg + i)->file.path = PATH[i];
		(user_arg + i)->offset = 0;
		(user_arg + i)->length = N;
		(user_arg + i)->data = (void *)malloc(N);

		if (is_write)
			(user_arg + i)->is_creation = 1;

		i++;
	}

	return user_arg;
}

/**
 * Verify the data has been
 * written as specified
 */
bool compare_content(tc_iovec *writev, tc_iovec *readv, int count)
{
	int i = 0;

	while (i < count) {
		if (memcmp((writev + i)->data, (readv + i)->data,
			   (writev + i)->length))
			return false;

		i++;
	}

	return true;
}

/**
 * TC-Read and Write test using
 * File path
 */
TEST(tc_test, WritevCanCreateFiles)
{
	const char *PATH[] = { "/tmp/WritevCanCreateFiles1.txt",
			       "/tmp/WritevCanCreateFiles2.txt",
			       "/tmp/WritevCanCreateFiles3.txt",
			       "/tmp/WritevCanCreateFiles4.txt" };
	char data[] = "abcd123";
	tc_res res;
	int count = 4;

	RemoveFiles(PATH, count);

	struct tc_iovec *writev = NULL;
	writev = set_iovec_file_paths(PATH, count, 1);
	EXPECT_FALSE(writev == NULL);

	res = tc_writev(writev, count, false);
	EXPECT_TRUE(res.okay);

	struct tc_iovec *readv = NULL;
	readv = set_iovec_file_paths(PATH, count, 0);
	EXPECT_FALSE(readv == NULL);

	res = tc_readv(readv, count, false);
	EXPECT_TRUE(res.okay);

	EXPECT_TRUE(compare_content(writev, readv, count));

	clear_iovec(writev, count);
	clear_iovec(readv, count);
}

/**
 * Set the TC I/O vector
 */
static tc_iovec *set_iovec_fd(int *fd, int count)
{
	int i = 0, N = 4096;
	tc_iovec *user_arg = NULL;

	user_arg = (tc_iovec *)calloc(count, sizeof(tc_iovec));

	while (i < count) {
		if (fd[i] < 0) {
			POSIX_WARN(
			    "set_iovec_fd() failed for fd at index : %d\n",
			    fd[i]);

			int indx = 0;
			while (indx < i) {
				free((user_arg + indx)->data);
				indx++;
			}
			free(user_arg);

			return NULL;
		}

		(user_arg + i)->file.type = TC_FILE_DESCRIPTOR;
		(user_arg + i)->file.fd = fd[i];
		(user_arg + i)->offset = 0;
		(user_arg + i)->length = N;
		(user_arg + i)->data = (void *)malloc(N);

		i++;
	}

	return user_arg;
}

/**
 * TC-Read and Write test using
 * File Descriptor
 */
TEST(tc_test, TestFileDesc)
{
	const char *PATH[] = { "/tmp/WritevCanCreateFiles1.txt",
			       "/tmp/WritevCanCreateFiles2.txt",
			       "/tmp/WritevCanCreateFiles3.txt",
			       "/tmp/WritevCanCreateFiles4.txt" };
	const int N = 7;
	char data[] = "abcd123";
	tc_res res;
	int i = 0, count = 4;
	int fd[count];
	int open_flags = O_RDWR | O_CREAT;

	RemoveFiles(PATH, 4);

	while (i < count) {
		fd[i] = open(PATH[i], open_flags);
		if (fd[i] < 0)
			POSIX_WARN("open failed for file %s\n", PATH[i]);
		i++;
	}

	struct tc_iovec *writev = NULL;
	writev = set_iovec_fd(fd, count);
	EXPECT_FALSE(writev == NULL);

	res = tc_writev(writev, count, false);
	EXPECT_TRUE(res.okay);

	struct tc_iovec *readv = NULL;
	readv = set_iovec_fd(fd, count);
	EXPECT_FALSE(readv == NULL);

	res = tc_readv(readv, count, false);
	EXPECT_TRUE(res.okay);

	EXPECT_TRUE(compare_content(writev, readv, count));

	clear_iovec(writev, count);
	clear_iovec(readv, count);

	i = 0;
	while (i < count) {
		close(fd[i]);
		i++;
	}
}

/**
 * Compare the attributes once set, to check if set properly
 */

bool compare(tc_attrs *usr, tc_attrs *check, int count)
{
	int i = 0;
	tc_attrs *written = NULL;
	tc_attrs *read = NULL;

	while (i < count) {

		written = usr + i;
		read = check + i;

		if (written->masks.has_mode) {
			if (!written->mode & read->mode) {
				POSIX_WARN("Mode does not match\n");
				POSIX_WARN(" %d %d\n", written->mode,
					   read->mode);

				return false;
			}
		}

		if (written->masks.has_rdev) {
			if (memcmp((void *)&(written->rdev),
				   (void *)&(read->rdev), sizeof(read->rdev))) {
				POSIX_WARN("rdev does not match\n");
				POSIX_WARN(" %d %d\n", written->rdev,
					   read->rdev);

				return false;
			}
		}

		if (written->masks.has_nlink) {
			if (written->nlink != read->nlink) {
				POSIX_WARN("nlink does not match\n");
				POSIX_WARN(" %d %d\n", written->nlink,
					   read->nlink);

				return false;
			}
		}

		if (written->masks.has_uid) {
			if (written->uid != read->uid) {
				POSIX_WARN("uid does not match\n");
				POSIX_WARN(" %d %d\n", written->uid, read->uid);

				return false;
			}
		}

		if (written->masks.has_gid) {
			if (written->gid != read->gid) {
				POSIX_WARN("gid does not match\n");
				POSIX_WARN(" %d %d\n", written->gid, read->gid);

				return false;
			}
		}

		if (written->masks.has_atime) {
			if (memcmp((void *)&(written->atime),
				   (void *)&(read->atime),
				   sizeof(read->atime))) {
				POSIX_WARN("atime does not match\n");
				POSIX_WARN(" %d %d\n", written->atime,
					   read->atime);

				return false;
			}
		}

		if (written->masks.has_mtime) {
			if (memcmp((void *)&(written->mtime),
				   (void *)&(read->mtime),
				   sizeof(read->mtime))) {
				POSIX_WARN("mtime does not match\n");
				POSIX_WARN(" %d %d\n", written->mtime,
					   read->mtime);

				return false;
			}
		}

		i++;
	}

	return true;
}

/**
 * Set the TC Attributes
 */
static tc_attrs *set_tc_attrs(const char **PATH, int count, bool isPath)
{
	if (count > 3) {
		POSIX_WARN("count should be less than 4\n");
		return NULL;
	}

	tc_attrs *change_attr = (tc_attrs *)calloc(count, sizeof(tc_attrs));
	tc_attrs_masks masks[3] = { 0 };
	int i = 0;

	uid_t uid[] = { 2711, 456, 789 };
	gid_t gid[] = { 87, 4566, 2311 };
	mode_t mode[] = { S_IRUSR | S_IRGRP | S_IROTH,
			  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH, S_IRWXU };
	size_t size[] = { 256, 56, 125 };
	time_t atime[] = { time(NULL), 1234, 567 };

	while (i < count) {

		if (PATH[i] == NULL) {
			free(change_attr);
			return NULL;
		}

		if (isPath) {
			(change_attr + i)->file.type = TC_FILE_PATH;
			(change_attr + i)->file.path = PATH[i];

		} else {
			(change_attr + i)->file.type = TC_FILE_DESCRIPTOR;
			(change_attr + i)->file.fd =
			    open(PATH[i], O_RDWR | O_CREAT);

			if ((change_attr + i)->file.fd < 0) {
				free(change_attr);
				return NULL;
			}
		}

		(change_attr + i)->mode = mode[i];
		(change_attr + i)->size = size[i];
		(change_attr + i)->uid = uid[i];
		(change_attr + i)->gid = gid[i];
		(change_attr + i)->atime = atime[i];
		(change_attr + i)->mtime = time(NULL);

		masks[i].has_mode = 1;
		masks[i].has_size = 1;
		masks[i].has_atime = 1;
		masks[i].has_mtime = 1;
		masks[i].has_uid = 1;
		masks[i].has_gid = 1;
		masks[i].has_rdev = 0;
		masks[i].has_nlink = 0;
		masks[i].has_ctime = 0;

		change_attr[i].masks = masks[i];

		i++;
	}

	return change_attr;
}

/* Set the TC attributes masks */
static void set_attr_masks(tc_attrs *write, tc_attrs *read, int count)
{
	int i = 0;
	tc_attrs *write_attr = NULL;
	tc_attrs *read_attr = NULL;

	while (i < count) {
		write_attr = write + i;
		read_attr = read + i;

		/* set tc_file */
		read_attr->file = write_attr->file;

		/* set masks */
		read_attr->masks.has_mode = write_attr->masks.has_mode;
		read_attr->masks.has_size = write_attr->masks.has_size;
		read_attr->masks.has_atime = write_attr->masks.has_atime;
		read_attr->masks.has_mtime = write_attr->masks.has_mtime;
		read_attr->masks.has_uid = write_attr->masks.has_uid;
		read_attr->masks.has_gid = write_attr->masks.has_gid;
		read_attr->masks.has_rdev = write_attr->masks.has_rdev;
		read_attr->masks.has_nlink = write_attr->masks.has_nlink;
		read_attr->masks.has_ctime = write_attr->masks.has_ctime;

		i++;
	}
}

/**
 * TC-Set/Get Attributes test
 * using File Path
 */
TEST(tc_test, AttrsTestPath)
{
	const char *PATH[] = { "/tmp/WritevCanCreateFiles1.txt",
			       "/tmp/WritevCanCreateFiles2.txt",
			       "/tmp/WritevCanCreateFiles3.txt" };
	tc_res res = { 0 };
	int count = 3;

	tc_attrs *write_attrs = NULL;
	write_attrs = set_tc_attrs(PATH, count, true);
	EXPECT_FALSE(write_attrs == NULL);

	res = tc_setattrsv(write_attrs, count, false);
	EXPECT_TRUE(res.okay);

	tc_attrs *read_attrs = (tc_attrs *)calloc(count, sizeof(tc_attrs));
	set_attr_masks(write_attrs, read_attrs, count);

	res = tc_getattrsv(read_attrs, count, false);
	EXPECT_TRUE(res.okay);

	EXPECT_TRUE(compare(write_attrs, read_attrs, count));

	free(write_attrs);
	free(read_attrs);
}

/*
 * TC-Set/Get Attributes test
 * using File Descriptor
 */
TEST(tc_test, AttrsTestFileDesc)
{
	const char *PATH[] = { "/tmp/WritevCanCreateFiles4.txt",
			       "/tmp/WritevCanCreateFiles5.txt",
			       "/tmp/WritevCanCreateFiles6.txt" };
	tc_res res = { 0 };
	int i = 0, count = 3;

	RemoveFiles(PATH, count);

	tc_attrs *write_attrs = NULL;
	write_attrs = set_tc_attrs(PATH, count, false);
	EXPECT_FALSE(write_attrs == NULL);

	res = tc_setattrsv(write_attrs, count, false);
	EXPECT_TRUE(res.okay);

	tc_attrs *read_attrs = (tc_attrs *)calloc(count, sizeof(tc_attrs));
	set_attr_masks(write_attrs, read_attrs, count);

	res = tc_getattrsv(read_attrs, count, false);
	EXPECT_TRUE(res.okay);

	EXPECT_TRUE(compare(write_attrs, read_attrs, count));

	while (i < count) {
		close((read_attrs + i)->file.fd);
		i++;
	}

	free(write_attrs);
	free(read_attrs);
}

/**
 * List Directory Contents Test
 */
TEST(tc_test, ListDirContents)
{
	tc_attrs *contents = (tc_attrs *)calloc(5, sizeof(tc_attrs));
	tc_attrs_masks masks = { 0 };
	int count = 0;

	masks.has_mode = 1;
	masks.has_size = 1;
	masks.has_atime = 1;
	masks.has_mtime = 1;
	masks.has_uid = 1;
	masks.has_gid = 1;
	masks.has_rdev = 1;
	masks.has_nlink = 1;
	masks.has_ctime = 1;

	contents->masks = masks;

	tc_res res = tc_listdir("/tmp/", masks, 5, &contents, &count);
	EXPECT_TRUE(res.okay);

	tc_attrs *read_attrs = (tc_attrs *)calloc(count, sizeof(tc_attrs));
	set_attr_masks(contents, read_attrs, count);

	res = tc_getattrsv(read_attrs, count, false);
	EXPECT_TRUE(res.okay);

	EXPECT_TRUE(compare(contents, read_attrs, count));

	free(contents);
	free(read_attrs);
}

/**
 * Rename File Test
 */
TEST(tc_test, RenameFile)
{
	int i = 0;
	const char *src_path[] = { "/tmp/WritevCanCreateFiles1.txt",
				   "/tmp/WritevCanCreateFiles2.txt",
				   "/tmp/WritevCanCreateFiles3.txt",
				   "/tmp/WritevCanCreateFiles4.txt" };

	const char *dest_path[] = { "/tmp/rename1.txt", "/tmp/rename2.txt",
				    "/tmp/rename3.txt", "/tmp/rename4.txt" };

	tc_file_pair *file = (tc_file_pair *)calloc(4, sizeof(tc_file_pair));

	while (i < 4) {
		file[i].src_file = tc_file_from_path(src_path[i]);
		file[i].dst_file = tc_file_from_path(dest_path[i]);
		i++;
	}

	tc_res res = tc_renamev(file, 4, false);

	EXPECT_TRUE(res.okay);

	free(file);
}

/**
 * Remove File Test
 */
TEST(tc_test, RemoveFile)
{
	int i = 0;
	const char *path[] = { "/tmp/rename1.txt", "/tmp/rename2.txt",
			       "/tmp/rename3.txt", "/tmp/rename4.txt" };

	tc_file *file = (tc_file *)calloc(4, sizeof(tc_file));

	while (i < 4) {
		file[i] = tc_file_from_path(path[i]);
		i++;
	}

	tc_res res = tc_removev(file, 4, false);

	EXPECT_TRUE(res.okay);

	free(file);
}

/**
 * Make Directory Test
 */
TEST(tc_test, MakeDirectory)
{
	int i = 0;
	mode_t mode[] = { S_IRWXU, S_IRUSR | S_IRGRP | S_IROTH,
			  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH };
	const char *path[] = { "/tmp/a", "/tmp/b", "/tmp/c" };

	tc_file *file = (tc_file *)calloc(3, sizeof(tc_file));

	RemoveDir(path, 3);

	while (i < 3) {

		(file + i)->path = path[i];
		(file + i)->type = TC_FILE_PATH;

		i++;
	}

	tc_res res = tc_mkdirv(file, mode, 3, false);

	EXPECT_TRUE(res.okay);

	free(file);
}
